#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>
#include <nfc/t4t/ndef_file.h>
#include <nfc_t4t_lib.h>
#include "nfctest.h"

K_MUTEX_DEFINE(nfc_lock);
K_CONDVAR_DEFINE(nfc_read_cv);
K_CONDVAR_DEFINE(nfc_write_cv);

static bool ndef_read_done;
static bool ndef_write_done;

static const uint8_t en_code[] = {'e', 'n'};

static const uint8_t testdata[] = { 'T', 'E', 'S', 'T', 'D', 'A', 'T', 'A' };

static uint8_t m_ndef_msg_buf[NDEF_MSG_BUF_SIZE];
static uint32_t m_ndef_len = NDEF_MSG_BUF_SIZE;

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

            if (data_length == 0)
            {
                printk("First write (NLEN=0), waiting for actual data...\n");
                break;
            }

            if (data_length > 0)
            {
                printk("new write data: ");
                for (size_t i = 0; i < NDEF_MSG_BUF_SIZE; i++)
                {
                    printk("%x, ", data[i]);
                }

                printk("\n");
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
static int build_text_ndef(uint8_t *buff, uint32_t size)
{
    int err;
    uint32_t ndef_size = nfc_t4t_ndef_file_msg_size_get(size);

    NFC_NDEF_TEXT_RECORD_DESC_DEF(text_record,
                    UTF_8,
                    en_code,
                    sizeof(en_code),
                    testdata,
                    sizeof(testdata));

    NFC_NDEF_MSG_DEF(nfc_text_msg, 1);

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
static int nfctest_send_data(void)
{
    int err;

    printk("Encoding NFC message...\n");

    size_t size_in = sizeof(m_ndef_msg_buf);

    if (build_text_ndef(m_ndef_msg_buf, size_in) < 0)
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
static int nfctest_receive_data()
{   
    int err;

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

    k_mutex_unlock(&nfc_lock);

    nfc_t4t_emulation_stop();
    printk("NDEF write done, emulation stopped\n");

    return 0;
}

int nfctest_setup(void)
{
    return nfc_t4t_setup(nfc_t4t_callback, NULL);
}

void nfctest(int mode)
{
    if (mode == 1)
    {
        nfctest_send_data();
    }
    else if (mode == 2)
    {
        nfctest_receive_data();
    }
    else
    {
        printk("Invalid mode\n");
    }
}