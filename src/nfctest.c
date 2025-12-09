#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <nfc_t2t_lib.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>
#include "nfctest.h"

static uint8_t ndef_msg_buf[NDEF_MSG_BUF_SIZE];
static uint32_t ndef_len;

static const uint8_t en_code[] = {'e', 'n'};
static const uint8_t testdata[] = { 'T', 'E', 'S', 'T', 'D', 'A', 'T', 'A' };

/* Buffer used to hold an NFC NDEF message. */
static uint8_t ndef_msg_buf[NDEF_MSG_BUF_SIZE];

static void nfc_callback(void *context,
			 nfc_t2t_event_t event,
			 const uint8_t *data,
			 size_t data_length)
{
	ARG_UNUSED(context);
	ARG_UNUSED(data);
	ARG_UNUSED(data_length);

    switch (event) 
    {
        case NFC_T2T_EVENT_FIELD_ON:
            printk("NFC field detected: phone is near\n");
            break;

        case NFC_T2T_EVENT_FIELD_OFF:
            printk("NFC field lost: phone moved away\n");
            break;

        default:
            printk("NFC event: %d\n", event);
            break;
    }
}

int nfctest_setup(void)
{
    return nfc_t2t_setup(nfc_callback, NULL);
}

static int build_text_ndef(uint8_t *buffer, uint32_t *data_len)
{
    NFC_NDEF_TEXT_RECORD_DESC_DEF(text_record,
                    UTF_8,
                    en_code,
                    sizeof(en_code),
                    testdata,
                    sizeof(testdata));

    NFC_NDEF_MSG_DEF(text_msg, 1);

    int err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(text_msg),
                    &NFC_NDEF_TEXT_RECORD_DESC(text_record));

    if (err < 0)
    {
        printk("Record add failed\n");
        return err;
    }

    *data_len = NDEF_MSG_BUF_SIZE;
    err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(text_msg),
                    buffer,
                    data_len);

    if (err < 0)
    {
        printk("Encode failed\n");
        return err;
    }

    return 0;
}


void nfctest_mode1(void)
{
    printk("NFC test mode 1 running\n");
}

int nfctest_send_data(void)
{
    printk("Encoding NFC message...\n");

    if (build_text_ndef(ndef_msg_buf, &ndef_len) < 0)
    {
        printk("Failed to build NDEF, cannot enoce message\n");
        return -1;
    }

    if (nfc_t2t_payload_set(ndef_msg_buf, ndef_len) < 0)
    {
        printk("Payload set failed\n");
        return -1;
    }

    if (nfc_t2t_emulation_start() < 0)
    {
        printk("Emulation start failed\n");
        return -1;
    }

    printk("NFC message ready, approach with phone\n");
    return 0;
}

void nfctest_receive_data(const char *data)
{
	printk("NFC data received\n");
}

void nfctest(int mode)
{
    if (mode == 1)
	{
		nfctest_mode1();
	}
    else if (mode == 2)
	{
        nfctest_send_data();
	}
    else if (mode == 3)
	{
		nfctest_receive_data("TESTDATA");
	}
    else
    {
        printk("Invalid mode\n");
    }
}