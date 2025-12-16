#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>
#include <nfc/t4t/ndef_file.h>
#include <nfc_t4t_lib.h>
#include <nfc/ndef/msg_parser.h>
#include "nfctest.h"

K_MUTEX_DEFINE(nfc_lock);
K_CONDVAR_DEFINE(nfc_read_cv);
K_CONDVAR_DEFINE(nfc_write_cv);

static bool ndef_read_done;
static bool ndef_write_done;

static const uint8_t en_code[] = {'e', 'n'};

static uint8_t m_ndef_msg_buf[NDEF_MSG_BUF_SIZE];
static uint32_t m_ndef_len = NDEF_MSG_BUF_SIZE;

static uint8_t nfc_test_rx_buf[NFC_TEST_RX_MAX];
static size_t  nfc_test_rx_len;

/* Handle incoming NDEF write with a TEXT record */
static int handle_ndef_write_text_record(const uint8_t *data, size_t data_length)
{
    int err;

    if (data_length <= 2)
    {
        printk("NDEF data too short\n");
        return -EINVAL;
    }

    /* Skip NLEN bytes */
    const uint8_t *ndef_msg_buff = data + 2;
    uint32_t ndef_msg_len = data_length - 2;

    struct nfc_ndef_bin_payload_desc bin_payload;
    struct nfc_ndef_record_desc record;
    enum nfc_ndef_record_location record_location;

    /* Parse first NDEF record */
    err = nfc_ndef_record_parse(&bin_payload,
                                &record,
                                &record_location,
                                ndef_msg_buff,
                                &ndef_msg_len);
    if (err)
    {
        printk("Record parse failed, err: %d\n", err);
        return err;
    }

    /* Ensure record is a TEXT record */
    if (record.tnf != TNF_WELL_KNOWN ||
        record.type_length != 1 ||
        record.type[0] != 'T')
    {

        printk("Not a TEXT record\n");
        return -ENOTSUP;
    }

    /* Get TEXT payload */
    const uint8_t *payload = bin_payload.payload;
    uint32_t payload_len = bin_payload.payload_length;

    if (payload_len < 1)
    {
        printk("Invalid TEXT payload length\n");
        return -EINVAL;
    }

    /* Extract language code length from status byte */
    uint8_t status = payload[0];
    uint8_t lang_len = status & 0x3F;

    if (payload_len < (1 + lang_len))
    {
        printk("Invalid language length\n");
        return -EINVAL;
    }

    /* Extract text */
    const uint8_t *text = &payload[1 + lang_len];
    uint32_t text_len = payload_len - 1 - lang_len;

    printk("TEXT RECORD RECEIVED\n");

    if (text_len >= NFC_TEST_RX_MAX)
    {
        text_len = NFC_TEST_RX_MAX - 1;
    }

    /* Copy received text to RX buffer and null-terminate */
    memcpy(nfc_test_rx_buf, text, text_len);
    nfc_test_rx_buf[text_len] = '\0';
    nfc_test_rx_len = text_len;

    return 0;
}

/*
 * NFC Type 4 Tag event callback.
 * Called by the NFC library whenever a field is detected, removed,
 * or an NDEF message is read/written by a phone.
 */
static void nfc_t4t_callback(void *context,
                    nfc_t4t_event_t event,
                    const uint8_t *data,
                    size_t data_length,
                    uint32_t flags)
{
    ARG_UNUSED(context);
    ARG_UNUSED(flags);

    k_mutex_lock(&nfc_lock, K_FOREVER);

    switch (event) 
    {
        case NFC_T4T_EVENT_FIELD_ON:
            printk("NFC field detected: phone is near\n");
            break;

        case NFC_T4T_EVENT_FIELD_OFF:
            printk("NFC field lost: phone moved away\n");
            break;

        case NFC_T4T_EVENT_NDEF_READ:
            printk("NDEF message read, length: %zu\n", data_length);

            ndef_read_done = true;
            k_condvar_signal(&nfc_read_cv);
             
            break;

        case NFC_T4T_EVENT_NDEF_UPDATED:
            printk("NDEF message updated, new length: %zu\n", data_length);
  
            if (data_length <= 2)
            {
                printk("First write (NLEN=0), waiting for actual data...\n");
                break;
            }

            ndef_write_done = true;
            k_condvar_signal(&nfc_write_cv);

            break;
        
        default:
            printk("NFC T4T event: %d\n", event);
            break;
    }

    k_mutex_unlock(&nfc_lock);
}

/*
 * Build a simple NDEF text record and encode it into the provided buffer.
 */
static int build_text_ndef(uint8_t *buff, uint32_t size, const uint8_t *data,
                           size_t data_length)
{
    int err;
    uint32_t ndef_size = nfc_t4t_ndef_file_msg_size_get(size);

    NFC_NDEF_TEXT_RECORD_DESC_DEF(text_record,
                    UTF_8,
                    en_code,
                    sizeof(en_code),
                    data,
                    data_length);

    NFC_NDEF_MSG_DEF(nfc_text_msg, MAX_REC_COUNT);

    err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_text_msg),
                    &NFC_NDEF_TEXT_RECORD_DESC(text_record));

    if (err < 0)
    {
        printk("Record add failed\n");
        return err;
    }
    
    err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(nfc_text_msg),
                              nfc_t4t_ndef_file_msg_get(buff),
                              &ndef_size);

    if (err < 0)
    {
        printk("Encode failed\n");
        return err;
    }

	err = nfc_t4t_ndef_file_encode(buff, &ndef_size);
	if (err) 
    {
        printk("nfc_t4t_ndef_file_encode() failed! err = %d\n", err);
		return err;
	}

    return 0;
}

/*
 * Start NFC tag emulation with a static (read-only) payload.
 * Wait until a phone reads the message.
 */
static int nfctest_send_data(const uint8_t *data, size_t data_length)
{
    int err;

    if (data == NULL || data_length == 0)
    {
        printk("No NFC data provided\n");
        return -EINVAL;
    }

    printk("Encoding NFC message...\n");

    memset(m_ndef_msg_buf, 0, sizeof(m_ndef_msg_buf));

    size_t size_in = sizeof(m_ndef_msg_buf);

    if (build_text_ndef(m_ndef_msg_buf, size_in, data, data_length) < 0)
    {
        printk("Failed to build NDEF, cannot encode message\n");
        return -1;
    }

    if (nfc_t4t_ndef_staticpayload_set(m_ndef_msg_buf, m_ndef_len) < 0)
    {
        printk("Payload set failed\n");
        return -1;
    }

    if (nfc_t4t_emulation_start() < 0)
    {
        printk("Emulation start failed\n");
        return -1;
    }

    printk("NFC message ready for Read, approach with phone\n");

    /* Wait for NDEF read event from callback */
    k_mutex_lock(&nfc_lock, K_FOREVER); //change K_FOREVER
    ndef_read_done = false;

    err = k_condvar_wait(&nfc_read_cv, &nfc_lock, K_FOREVER);
    if (err == -EAGAIN)
    {
        printk("Timeout while waiting for NDEF read\n");
    }

    k_mutex_unlock(&nfc_lock);

    nfc_t4t_emulation_stop();
    printk("NDEF read done, emulation stopped\n");

    return 0;
}

/*
 * Start NFC tag emulation in read/write mode.
 * Wait until a phone writes a new NDEF message.
 */
static int nfctest_receive_data(const uint8_t *data, size_t data_length)
{   
    int err;

    memset(m_ndef_msg_buf, 0, sizeof(m_ndef_msg_buf));

    if (nfc_t4t_ndef_rwpayload_set(m_ndef_msg_buf, m_ndef_len) < 0)
    {
        printk("Payload set failed\n");
        return -1;
    }

    if (nfc_t4t_emulation_start() < 0)
    {
        printk("Emulation start failed\n");
        return -1;
    }

    printk("NFC message ready for Read/Write, approach with phone\n");

    /* Wait for NDEF write event from callback*/
    k_mutex_lock(&nfc_lock, K_FOREVER); //change K_FOREVER
    ndef_write_done = false;

    err = k_condvar_wait(&nfc_write_cv, &nfc_lock, K_FOREVER);
    if (err == -EAGAIN)
    {
        printk("Timeout while waiting for NDEF write\n");
    }   

    int parse_ret;

    parse_ret = handle_ndef_write_text_record(m_ndef_msg_buf, m_ndef_len);

    k_mutex_unlock(&nfc_lock);

    nfc_t4t_emulation_stop();
    printk("NDEF write done, emulation stopped\n");

    if (parse_ret < 0)
    {
        return parse_ret;
    }

    return 0;
}

int nfctest_setup(void)
{
    return nfc_t4t_setup(nfc_t4t_callback, NULL);
}

int nfctest(int mode, const uint8_t *data, size_t data_length)
{
    if (mode == 1)
    {
        printk("NFCTEST MODE 1 START\n");
        return nfctest_send_data(data, data_length);
    }
    else if (mode == 2)
    {
        printk("NFCTEST MODE 2 START\n");
        return nfctest_receive_data(data, data_length);
    }
    else
    {
        printk("Invalid mode\n");
        return -EINVAL;
    }
}

int nfctest_get_rx_text(uint8_t *buf, size_t buf_size, size_t *out_len)
{
    if (nfc_test_rx_len == 0)
    {
        return -ENOENT;
    }

    if (buf_size < nfc_test_rx_len + 1)
    {
        return -ENOBUFS;
    }

    memcpy(buf, nfc_test_rx_buf, nfc_test_rx_len + 1);
    *out_len = nfc_test_rx_len;

    return 0;
}