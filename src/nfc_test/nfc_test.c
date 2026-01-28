#include <zephyr/kernel.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>
#include <nfc/t4t/ndef_file.h>
#include <nfc_t4t_lib.h>
#include <nfc/ndef/msg_parser.h>
#include <zephyr/logging/log.h>
#include "nfc_test.h"

LOG_MODULE_REGISTER(nfctest);

K_MUTEX_DEFINE(nfc_lock);
K_CONDVAR_DEFINE(nfc_read_cv);
K_CONDVAR_DEFINE(nfc_write_cv);

static bool m_ndef_operation_done;
static bool m_field_off;

static uint8_t m_ndef_msg_buf[NDEF_MSG_BUF_SIZE];
static uint32_t m_ndef_len = NDEF_MSG_BUF_SIZE;

static bool nfc_t4t_initialized;

typedef enum
{
    NDEF_OP_NONE,
    NDEF_TEST_READ,
    NDEF_TEST_WRITE
} ndef_op;

static ndef_op m_current_op = NDEF_OP_NONE;

/* Parse a TEXT NDEF record written by an NFC reader/writer (e.g. a smartphone) and extract its payload */
static int handle_ndef_text_record(const uint8_t *data, size_t data_length, uint8_t *payload_buf,
                                         size_t *payload_len)
{
    int err;

    if (!data || data_length <= 2)
    {
        LOG_WRN("NDEF data too short");
        return -EINVAL;
    }

    /* Skip NLEN (2 bytes) */
    const uint8_t *ndef_msg_buf = data + 2;
    uint32_t ndef_msg_len = data_length - 2;

    struct nfc_ndef_bin_payload_desc bin_payload;
    struct nfc_ndef_record_desc record;
    enum nfc_ndef_record_location record_location;

    /* Parse first NDEF record */
    err = nfc_ndef_record_parse(&bin_payload,
                                &record,
                                &record_location,
                                ndef_msg_buf,
                                &ndef_msg_len);
    if (err)
    {
        LOG_ERR("NDEF record parse failed (%d)", err);
        return err;
    }

    /* Verify TEXT record */
    if (record.tnf != TNF_WELL_KNOWN ||
        record.type_length != 1 ||
        record.type[0] != 'T')
    {
        LOG_WRN("Not a TEXT record");
        return -ENOTSUP;
    }

    /* Validate payload */
    if (bin_payload.payload_length < 1)
    {
        LOG_ERR("Invalid TEXT payload length");
        return -EINVAL;
    }

    const uint8_t *payload = bin_payload.payload;
    uint32_t payload_length = bin_payload.payload_length;

    uint8_t status = payload[0];
    uint8_t lang_len = status & 0x3F;

    if (payload_length < (1 + lang_len))
    {
        LOG_ERR("Invalid language code length");
        return -EINVAL;
    }

    const uint8_t *text = &payload[1 + lang_len];
    uint32_t text_len = payload_length - 1 - lang_len;

    uint32_t copy_len = MIN(text_len, NFCTEST_PAYLOAD_MAX - 1);

    memcpy(payload_buf, text, copy_len);
    payload_buf[copy_len] = '\0';
    *payload_len = copy_len;

    LOG_INF("NFC RX TEXT: %s", payload_buf);

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
            LOG_INF("NFC field detected: phone is near");
            break;

        case NFC_T4T_EVENT_FIELD_OFF:
            LOG_INF("NFC field lost: phone moved away");
            if (m_ndef_operation_done)
            {
                m_field_off = true;
            }
            
            k_condvar_signal(&nfc_read_cv);
            k_condvar_signal(&nfc_write_cv);
            break;

        case NFC_T4T_EVENT_NDEF_READ:
            LOG_INF("NDEF message read, length: %zu", data_length);


            if (m_current_op == NDEF_TEST_READ)
            {
                m_ndef_operation_done = true;
                k_condvar_signal(&nfc_read_cv);
            }
            break;

        case NFC_T4T_EVENT_NDEF_UPDATED:
            LOG_INF("NDEF message updated, new length: %zu", data_length);
  
            if (data_length <= 2)
            {
                LOG_DBG("First write (NLEN=0), waiting for actual data...");
                break;
            }

            if (m_current_op == NDEF_TEST_WRITE)
            {
                m_ndef_operation_done = true;

                k_condvar_signal(&nfc_write_cv);
            }

            break;
        
        default:
            LOG_INF("NFC T4T event: %d", event);
            break;
    }

    k_mutex_unlock(&nfc_lock);
}

/*
 * Build a simple NDEF text record and encode it into the provided buffer.
 */
static int build_text_ndef(uint8_t *buff, uint32_t size, const uint8_t *data,
                           size_t data_length, uint32_t *out_length)
{
    int err;
    uint32_t ndef_size = nfc_t4t_ndef_file_msg_size_get(size);
    const uint8_t en_code[] = {'e', 'n'};

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
        LOG_ERR("Record add failed");
        return err;
    }
    
    err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(nfc_text_msg),
                              nfc_t4t_ndef_file_msg_get(buff),
                              &ndef_size);

    if (err < 0)
    {
        LOG_ERR("Encode failed");
        return err;
    }

	err = nfc_t4t_ndef_file_encode(buff, &ndef_size);
	if (err) 
    {
        LOG_ERR("nfc_t4t_ndef_file_encode() failed! err = %d", err);
		return err;
	}

    *out_length = ndef_size;

    return 0;
}

/*
 * Start NFC tag emulation with a static (read-only) payload.
 * Wait until a phone reads the message.
 */
static int nfctest_send_data(const uint8_t *data, size_t data_length)
{
    if (data == NULL || data_length == 0)
    {
        LOG_ERR("No NFC data provided");
        return -EINVAL;
    }

    LOG_INF("Encoding NFC message...");

    memset(m_ndef_msg_buf, 0, sizeof(m_ndef_msg_buf));

    size_t size_in = sizeof(m_ndef_msg_buf);
    uint32_t encoded_len = 0;

    if (build_text_ndef(m_ndef_msg_buf, size_in, data, data_length, &encoded_len) < 0)
    {
        LOG_ERR("Failed to build NDEF, cannot encode message");
        return -EIO;
    }
    m_ndef_len = encoded_len;

    if (nfc_t4t_ndef_staticpayload_set(m_ndef_msg_buf, m_ndef_len) < 0)
    {
        LOG_ERR("Payload set failed");
        return -EIO;
    }

    if (nfc_t4t_emulation_start() < 0)
    {
        LOG_ERR("Emulation start failed");
        return -EIO;
    }

    LOG_INF("NFC message ready for Read, approach with phone");

    /* Wait for NDEF read event from callback */
    k_mutex_lock(&nfc_lock, K_FOREVER);
    m_current_op = NDEF_TEST_READ;
    m_ndef_operation_done = false;
    m_field_off = false;

    int64_t start = k_uptime_get();
    int64_t timeout_ms = 5000;

    while (!m_ndef_operation_done) 
    {
        if ((k_uptime_get() - start) >= timeout_ms) 
        {
            LOG_ERR("Timeout waiting NDEF read");
            k_mutex_unlock(&nfc_lock);
            nfc_t4t_emulation_stop();
            return -ETIMEDOUT;
        }

        int64_t remaining = timeout_ms - (k_uptime_get() - start);
        k_condvar_wait(&nfc_read_cv,
                    &nfc_lock,
                    K_MSEC(remaining));
    }

    while (!m_field_off) 
    {
        if ((k_uptime_get() - start) >= timeout_ms) 
        {
            LOG_ERR("Timeout waiting field off");
            k_mutex_unlock(&nfc_lock);
            nfc_t4t_emulation_stop();
            return -ETIMEDOUT;
        }

        int64_t remaining = timeout_ms - (k_uptime_get() - start);
        k_condvar_wait(&nfc_read_cv,
                    &nfc_lock,
                    K_MSEC(remaining));
    }

    m_current_op = NDEF_OP_NONE;
    k_mutex_unlock(&nfc_lock);

    nfc_t4t_emulation_stop();
    LOG_INF("NDEF read done, emulation stopped");

    return 0;
}

/*
 * Start NFC tag emulation in read/write mode.
 * Wait until a phone writes a new NDEF message.
 */
static int nfctest_receive_data(const uint8_t *data, size_t *data_length)
{   
    int err;

    memset(m_ndef_msg_buf, 0, sizeof(m_ndef_msg_buf));
    m_ndef_len = sizeof(m_ndef_msg_buf);

    if (nfc_t4t_ndef_rwpayload_set(m_ndef_msg_buf, m_ndef_len) < 0)
    {
        LOG_ERR("Payload set failed");
        return -1;
    }

    if (nfc_t4t_emulation_start() < 0)
    {
        LOG_ERR("Emulation start failed");
        return -1;
    }

    LOG_INF("NFC message ready for Read/Write, approach with phone");

    /* Wait for NDEF write event from callback*/
    k_mutex_lock(&nfc_lock, K_FOREVER);
    m_current_op = NDEF_TEST_WRITE;
    m_ndef_operation_done = false;
    m_field_off = false;

    int64_t start = k_uptime_get();
    int64_t timeout_ms = 5000; 

    while (!m_ndef_operation_done) 
    {
        if ((k_uptime_get() - start) >= timeout_ms) 
        {
            LOG_ERR("Timeout waiting NDEF write");
            k_mutex_unlock(&nfc_lock);
            nfc_t4t_emulation_stop();
            return -ETIMEDOUT;
        }

        int64_t remaining = timeout_ms - (k_uptime_get() - start);
        k_condvar_wait(&nfc_write_cv,
                    &nfc_lock,
                    K_MSEC(remaining));
    }

    while (!m_field_off) 
    {
        if ((k_uptime_get() - start) >= timeout_ms)
        {
            LOG_ERR("Timeout waiting field off");
            k_mutex_unlock(&nfc_lock);
            nfc_t4t_emulation_stop();
            return -ETIMEDOUT;
        }

        int64_t remaining = timeout_ms - (k_uptime_get() - start);
        k_condvar_wait(&nfc_write_cv,
                    &nfc_lock,
                    K_MSEC(remaining));
    }

    m_current_op = NDEF_OP_NONE;
    k_mutex_unlock(&nfc_lock);

    nfc_t4t_emulation_stop();
    LOG_INF("NDEF write done, emulation stopped");

    err = handle_ndef_text_record(m_ndef_msg_buf, m_ndef_len, (uint8_t *)data,
                                          data_length);
    
    if (err < 0)
    {
        return err;
    }

    return 0;
}

static int nfctest_t4t_setup(void)
{
    if (nfc_t4t_initialized) {
        return 0;
    }

    int err = nfc_t4t_setup(nfc_t4t_callback, NULL);
    if (err < 0) {
        LOG_ERR("nfc_t4t_setup failed (%d)", err);
        return err;
    }

    nfc_t4t_initialized = true;
    LOG_INF("NFC T4T initialized");

    return 0;
}

int nfctest(int mode, uint8_t *data, size_t *data_length)
{
    if (!data || !data_length)
    {
        return -EINVAL;
    }

    if (mode == 1)
    {
        LOG_INF("NFCTEST MODE 1 START");

        if (*data_length == 0)
        {
            LOG_WRN("No data to send");
            return -EINVAL;
        }

        int err = nfctest_t4t_setup();
        if (err < 0) {
            return err;
        }

        return nfctest_send_data(data, *data_length);
    }
    else if (mode == 2)
    {
        LOG_INF("NFCTEST MODE 2 START");

        memset(data, 0, NFCTEST_PAYLOAD_MAX);
        *data_length = 0;

        int err = nfctest_t4t_setup();
        if (err < 0) {
            return err;
        }

        int ret = nfctest_receive_data(data, data_length);

        if (ret == 0)
        {
            LOG_INF("RX PAYLOAD: %s", data);
        }

        return ret;
    }

    return -EINVAL;
}
