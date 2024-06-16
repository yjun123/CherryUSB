/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "usbd_core.h"
#include "usbd_video.h"
#include "cherryusb_mjpeg.h"

#define VIDEO_IN_EP  0x81
#define VIDEO_INT_EP 0x83

#ifdef CONFIG_USB_HS
#define MAX_PAYLOAD_SIZE  1024 // for high speed with one transcations every one micro frame
#define VIDEO_PACKET_SIZE (unsigned int)(((MAX_PAYLOAD_SIZE / 1)) | (0x00 << 11))

// #define MAX_PAYLOAD_SIZE  2048 // for high speed with two transcations every one micro frame
// #define VIDEO_PACKET_SIZE (unsigned int)(((MAX_PAYLOAD_SIZE / 2)) | (0x01 << 11))

// #define MAX_PAYLOAD_SIZE  3072 // for high speed with three transcations every one micro frame
// #define VIDEO_PACKET_SIZE (unsigned int)(((MAX_PAYLOAD_SIZE / 3)) | (0x02 << 11))

#else
#define MAX_PAYLOAD_SIZE  1020
#define VIDEO_PACKET_SIZE (unsigned int)(((MAX_PAYLOAD_SIZE / 1)) | (0x00 << 11))
#endif

#define WIDTH  (unsigned int)(640)
#define HEIGHT (unsigned int)(480)

#define CAM_FPS        (30)
#define INTERVAL       (unsigned long)(10000000 / CAM_FPS)
#define MIN_BIT_RATE   (unsigned long)(WIDTH * HEIGHT * 16 * CAM_FPS) //16 bit
#define MAX_BIT_RATE   (unsigned long)(WIDTH * HEIGHT * 16 * CAM_FPS)
#define MAX_FRAME_SIZE (unsigned long)(WIDTH * HEIGHT * 2)

#define VS_HEADER_SIZ (unsigned int)(VIDEO_SIZEOF_VS_INPUT_HEADER_DESC(1,1) + VIDEO_SIZEOF_VS_FORMAT_MJPEG_DESC + VIDEO_SIZEOF_VS_FRAME_MJPEG_DESC(1))

#define VC_XU_DESCRIPTOR_LEN VIDEO_SIZEOF_VC_EXTENSION_UNIT_DESC(1, 1)
#define USB_VIDEO_DESC_SIZ (unsigned long)(9 +                            \
                                           VIDEO_VC_NOEP_DESCRIPTOR_LEN + \
                                           VC_XU_DESCRIPTOR_LEN + \
                                           9 +                            \
                                           VS_HEADER_SIZ +                \
                                           9 +                            \
                                           7)

#define USBD_VID           0xffff
#define USBD_PID           0xffff
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

// 用户自由指定
#define VIDEO_GUID_XU 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
// 需要和usbd_video中的定义对齐
#define VIDEO_ID_XU 7

const uint8_t video_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xef, 0x02, 0x01, USBD_VID, USBD_PID, 0x0001, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_VIDEO_DESC_SIZ, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    //VIDEO_VC_DESCRIPTOR_INIT(0x00, VIDEO_INT_EP, 0x0100, VIDEO_VC_TERMINAL_LEN, 48000000, 0x02),
    VIDEO_VC_NOEP_DESCRIPTOR_INIT(0x00, VIDEO_INT_EP, 0x0100, VIDEO_VC_TERMINAL_LEN + VC_XU_DESCRIPTOR_LEN, 48000000, 0x02),
    VIDEO_VC_EXTENSION_UNIT_DESCRIPTOR_INIT(VIDEO_ID_XU, VIDEO_GUID_XU),
    VIDEO_VS_DESCRIPTOR_INIT(0x01, 0x00, 0x00),
    VIDEO_VS_INPUT_HEADER_DESCRIPTOR_INIT(0x01, VS_HEADER_SIZ, VIDEO_IN_EP, 0x00),
    VIDEO_VS_FORMAT_MJPEG_DESCRIPTOR_INIT(0x01, 0x01),
    VIDEO_VS_FRAME_MJPEG_DESCRIPTOR_INIT(0x01, WIDTH, HEIGHT, MIN_BIT_RATE, MAX_BIT_RATE, MAX_FRAME_SIZE, DBVAL(INTERVAL), 0x01, DBVAL(INTERVAL)),
    VIDEO_VS_DESCRIPTOR_INIT(0x01, 0x01, 0x01),
    VIDEO_VS_ISO_ENDPOINT_DESCRIPTOR_INIT(VIDEO_IN_EP, VIDEO_PACKET_SIZE, 0x01),

    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x26,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'U', 0x00,                  /* wcChar10 */
    'V', 0x00,                  /* wcChar11 */
    'C', 0x00,                  /* wcChar12 */
    ' ', 0x00,                  /* wcChar13 */
    'D', 0x00,                  /* wcChar14 */
    'E', 0x00,                  /* wcChar15 */
    'M', 0x00,                  /* wcChar16 */
    'O', 0x00,                  /* wcChar17 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '1', 0x00,                  /* wcChar3 */
    '0', 0x00,                  /* wcChar4 */
    '3', 0x00,                  /* wcChar5 */
    '1', 0x00,                  /* wcChar6 */
    '0', 0x00,                  /* wcChar7 */
    '0', 0x00,                  /* wcChar8 */
    '0', 0x00,                  /* wcChar9 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
#endif
    0x00
};

volatile bool tx_flag = 0;
volatile bool iso_tx_busy = false;

static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event) {
        case USBD_EVENT_RESET:
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;
        case USBD_EVENT_CONFIGURED:
            tx_flag = 0;
            iso_tx_busy = false;
            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

void usbd_video_open(uint8_t busid, uint8_t intf)
{
    tx_flag = 1;
    USB_LOG_RAW("OPEN\r\n");
    iso_tx_busy = false;
}
void usbd_video_close(uint8_t busid, uint8_t intf)
{
    USB_LOG_RAW("CLOSE\r\n");
    tx_flag = 0;
    iso_tx_busy = false;
}

void usbd_video_iso_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    //USB_LOG_RAW("actual in len:%d\r\n", nbytes);
    iso_tx_busy = false;
}

static struct usbd_endpoint video_in_ep = {
    .ep_cb = usbd_video_iso_callback,
    .ep_addr = VIDEO_IN_EP
};

struct usbd_interface intf0;
struct usbd_interface intf1;

void video_init(uint8_t busid, uint32_t reg_base)
{
    usbd_desc_register(busid, video_descriptor);
    usbd_add_interface(busid, usbd_video_init_intf(busid, &intf0, INTERVAL, MAX_FRAME_SIZE, MAX_PAYLOAD_SIZE));
    usbd_add_interface(busid, usbd_video_init_intf(busid, &intf1, INTERVAL, MAX_FRAME_SIZE, MAX_PAYLOAD_SIZE));
    usbd_add_endpoint(busid, &video_in_ep);

    usbd_initialize(busid, reg_base, usbd_event_handler);
}

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t packet_buffer[40 * 1024];

void video_test(uint8_t busid)
{
    uint32_t out_len;
    uint32_t packets;

    (void)packets;
    memset(packet_buffer, 0, 40 * 1024);
    while (1) {
        if (tx_flag) {
            packets = usbd_video_mjpeg_payload_fill(busid, (uint8_t *)cherryusb_mjpeg, sizeof(cherryusb_mjpeg), packet_buffer, &out_len);
#if 0
            iso_tx_busy = true;
            usbd_ep_start_write(busid, VIDEO_IN_EP, packet_buffer, out_len);
            while (iso_tx_busy) {
                if (tx_flag == 0) {
                    break;
                }
            }
#else
            /* dwc2 must use this method */
            for (uint32_t i = 0; i < packets; i++) {
                if (i == (packets - 1)) {
                    iso_tx_busy = true;
                    usbd_ep_start_write(busid, VIDEO_IN_EP, &packet_buffer[i * MAX_PAYLOAD_SIZE], out_len - (packets - 1) * MAX_PAYLOAD_SIZE);
                    while (iso_tx_busy) {
                        if (tx_flag == 0) {
                            break;
                        }
                    }
                } else {
                    iso_tx_busy = true;
                    usbd_ep_start_write(busid, VIDEO_IN_EP, &packet_buffer[i * MAX_PAYLOAD_SIZE], MAX_PAYLOAD_SIZE);
                    while (iso_tx_busy) {
                        if (tx_flag == 0) {
                            break;
                        }
                    }
                }
            }
#endif
        }
    }
}

int video_control_extension_unit_request_handler(uint8_t entity_id, uint8_t control_selector, 
                                            uint8_t bRequest,uint8_t **data, uint32_t *len)
{
    USB_LOG_INFO("id %d, cs %d, bRequest %x, data %x, len %d\r\n",entity_id, control_selector, bRequest, *data, *len);

    if (entity_id != VIDEO_ID_XU || control_selector != 1)
        return 0;
    
#define XU_DATA_LEN 60
    static uint8_t cur_buf[XU_DATA_LEN] = {};
    switch (bRequest)
    {
    case VIDEO_REQUEST_GET_CUR:
        memcpy(*data, cur_buf, XU_DATA_LEN);
        *len = XU_DATA_LEN;
        break;
    case VIDEO_REQUEST_SET_CUR:
        for (int i = 0; i < *len; i++)
            USB_LOG_RAW("0x%x ", (*data)[i]);
        USB_LOG_RAW("\r\n");
        memset(cur_buf, 0x00, XU_DATA_LEN);
        memcpy(cur_buf, *data, *len);
        break;
    case VIDEO_REQUEST_GET_MIN:
    case VIDEO_REQUEST_GET_MAX:
    case VIDEO_REQUEST_GET_DEF:
        memset(*data, 0x00, XU_DATA_LEN);
        *len = XU_DATA_LEN;
        break;
    case VIDEO_REQUEST_GET_INFO:
        (*data)[0] = 0x03; // SET/GET support
        *len = 1;
        break;
    case VIDEO_REQUEST_GET_LEN:
        // 4.2.2.4 Extension Unit Control Requests
        /* When issuing the GET_LEN request, 
        the wLength field shall always be
        set to a value of 2 bytes. */
        (*data)[0] = XU_DATA_LEN;
        (*data)[1] = 0x00;
        *len = 2;
        break;
    default:
        break;
    }

    return 0;
}
