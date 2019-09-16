#include "face_lib.h"

#include "global_build_info_version.h"

#include "face_cb.h"
#include "camera.h"

#include "net_8285.h"
#include "http_file.h"

#include "lcd.h"
#include "lcd_dis.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static uint64_t last_open_relay_time_in_s = 0;
static volatile uint8_t relay_open_flag = 0;

static void uart_send(char *buf, size_t len);

face_recognition_cfg_t face_recognition_cfg = {
    .check_ir_face = 1,
    .auto_out_fea = 0,
    .detect_threshold = 0.0,
    .compare_threshold = 0.0,
};

face_lib_callback_t face_recognition_cb = (face_lib_callback_t)
{
#if CONFIG_PROTO_OVER_NET
    .proto_send = spi_8266_mqtt_send,
#else
    .proto_send = uart_send,
#endif
    .proto_record_face = protocol_record_face,

    .detected_face_cb = detected_face_cb,
    .fake_face_cb = fake_face_cb,
    .pass_face_cb = pass_face_cb,
    .lcd_refresh_cb = lcd_refresh_cb,
    .lcd_convert_cb = lcd_convert_cb,
};

static void uart_send(char *buf, size_t len)
{
    uart_send_data(PROTOCOL_UART_NUM, buf, len);
}

static void open_relay(void)
{
    uint64_t tim = sysctl_get_time_us();

    gpiohs_set_pin(CONFIG_RELAY_LOWX_GPIOHS_NUM, 1);
    gpiohs_set_pin(CONFIG_RELAY_HIGH_GPIOHS_NUM, 0);

    last_open_relay_time_in_s = tim / 1000 / 1000;
    relay_open_flag = 1;
}

static void close_relay(void)
{
    gpiohs_set_pin(CONFIG_RELAY_LOWX_GPIOHS_NUM, 0);
    gpiohs_set_pin(CONFIG_RELAY_HIGH_GPIOHS_NUM, 1);
}

static uint64_t last_pass_time = 0;

/********************* Add your callback code here *********************/
void face_pass_callback(face_obj_t *obj, uint32_t total, uint32_t current, uint64_t *time)
{
    face_info_t info;
    uint64_t tim = 0;

    tim = sysctl_get_time_us();

    if (g_board_cfg.brd_soft_cfg.cfg.out_interval_ms != 0)
    {
        if (((tim - last_pass_time) / 1000) < g_board_cfg.brd_soft_cfg.cfg.out_interval_ms)
        {
            printk("last face pass time too short\r\n");
            return;
        }
    }

    last_pass_time = tim;

    /* output feature */
    if (g_board_cfg.brd_soft_cfg.cfg.auto_out_fea)
    {
        protocol_send_face_info(obj,
                                0, NULL, obj->feature,
                                total, current, time);
        open_relay(); //open when have face
    }
    else
    {
        if (obj->score > g_board_cfg.brd_soft_cfg.out_threshold)
        {
            open_relay(); //open when score > gate
            if (flash_get_saved_faceinfo(&info, obj->index) == 0)
            {
                if (g_board_cfg.brd_soft_cfg.cfg.out_fea == 2)
                {
                    //output real time feature
                    protocol_send_face_info(obj,
                                            obj->score, info.uid, g_board_cfg.brd_soft_cfg.cfg.out_fea ? obj->feature : NULL,
                                            total, current, time);
                }
                else
                {
                    //output stored in flash face feature
                    face_fea_t *face_fea = (face_fea_t *)&(info.info);
                    protocol_send_face_info(obj,
                                            obj->score, info.uid,
                                            g_board_cfg.brd_soft_cfg.cfg.out_fea ? (face_fea->stat == 1) ? face_fea->fea_ir : face_fea->fea_rgb : NULL,
                                            total, current, time);
                }
            }
            else
            {
                printk("index error!\r\n");
            }
        }
        else
        {
            printf("face score not pass\r\n");
        }
    }

    if (g_board_cfg.brd_soft_cfg.cfg.auto_out_fea == 0)
    {
        lcd_draw_pass();
    }
    return;
}

//recv: {"version":1,"type":"test"}
//send: {"version":1,"type":"test","code":0,"msg":"test"}
void test_cmd(cJSON *root)
{
    cJSON *ret = protocol_gen_header("test", 0, "test");
    if (ret)
    {
        protocol_send(ret);
    }
    cJSON_Delete(ret);
    return;
}

//recv: {"version":1,"type":"test2","log_tx":10}
//send: {"1":1,"type":"test2","code":0,"msg":"test2"}
void test2_cmd(cJSON *root)
{
    cJSON *ret = NULL;
    cJSON *tmp = NULL;

    tmp = cJSON_GetObjectItem(root, "log_tx");
    if (tmp == NULL)
    {
        printk("no log_tx recv\r\n");
    }
    else
    {
        printk("log_tx:%d\r\n", tmp->valueint);
    }

    ret = protocol_gen_header("test2", 0, "test2");
    if (ret)
    {
        protocol_send(ret);
    }
    cJSON_Delete(ret);
    return;
}

protocol_custom_cb_t user_custom_cmd[] = {
    {.cmd = "test", .cmd_cb = test_cmd},
    {.cmd = "test2", .cmd_cb = test2_cmd},
};

int main(void)
{
    uint64_t tim = 0, last_mqtt_check_tim = 0;

    board_init();
    // sd_init_fatfs();
    face_lib_init_module();

    face_cb_init();
    lcd_dis_list_init();

    printk("firmware version:\r\n%d.%d.%d\r\n", BUILD_VERSION_MAJOR, BUILD_VERSION_MINOR, BUILD_VERSION_MICRO);
    printk("face_lib_version:\r\n%s\r\n", face_lib_version());

    /*load cfg from flash*/
    if (flash_load_cfg(&g_board_cfg) == 0)
    {
        printk("load cfg failed,save default config\r\n");

        flash_cfg_set_default(&g_board_cfg);

        if (flash_save_cfg(&g_board_cfg) == 0)
        {
            printk("save g_board_cfg failed!\r\n");
        }
    }

    flash_cfg_print(&g_board_cfg);

    face_lib_regisiter_callback(&face_recognition_cb);

    /* init device */
#if (CONFIG_PROTO_OVER_NET == 0)
    protocol_regesiter_user_cb(&user_custom_cmd[0], sizeof(user_custom_cmd) / sizeof(user_custom_cmd[0]));
#endif

    protocol_init_device(&g_board_cfg);

#if CONFIG_WIFI_ENABLE
    /* init 8285 */
    spi_8266_init_device();

    if (strlen(g_board_cfg.wifi_ssid) > 0 && strlen(g_board_cfg.wifi_passwd) >= 8)
    {
        lcd_display_image_alpha(IMG_CONNING_ADDR, 0);
        g_net_status = spi_8266_connect_ap(g_board_cfg.wifi_ssid, g_board_cfg.wifi_passwd, 2);
        if (g_net_status)
        {
            lcd_display_image_alpha(IMG_CONN_SUCC_ADDR, 0);
#if CONFIG_NET_DEMO_MQTT
            spi_8266_mqtt_init();
#endif
        }
        else
        {
            lcd_display_image_alpha(IMG_CONN_FAILED_ADDR, 0);
            printk("wifi config maybe error,we can not connect to it!\r\n");
        }
    }
#endif

    protocol_send_init_done();
    while (1)
    {
#if CONFIG_NET_DEMO_HTTP_GET

        uint8_t http_header[1024];

        if (g_net_status)
        {
            dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);

            uint64_t tm = sysctl_get_time_us();

            uint32_t get_recv_len = http_get_file(
                "https://fdvad021asfd8q.oss-cn-hangzhou.aliyuncs.com/logo.jpg",
                NULL,
                http_header,
                sizeof(http_header),
                display_image,
                sizeof(display_image));

            uint64_t tt = sysctl_get_time_us() - tm;

            printk("http get use %ld us\r\n", tt);

            float time_s = (float)((float)tt / 1000.0 / 1000.0);
            float file_kb = (float)((float)get_recv_len / 1024.0);

            printf("about %f KB/s\r\n", (float)(file_kb / time_s));

            printk("get_recv_len:%d\r\n", get_recv_len);
            printk("get recv hdr:%s\r\n", http_header);

            if (get_recv_len > 0)
            {
                jpeg_decode_image_t *jpeg = NULL;

                jpeg = pico_jpeg_decode(kpu_image[0], display_image, get_recv_len, 1);

                if (jpeg)
                {
                    printk("img width:%d\theight:%d\r\n", jpeg->width, jpeg->height);
                    if (jpeg->width == 240 && jpeg->height == 240)
                    {
                        convert_jpeg_img_order(jpeg);

                        lcd_draw_picture(0, 0, 240, 240, (uint32_t *)jpeg->img_data);
                    }
                }
#if CONFIG_NET_DEMO_HTTP_POST
                uint8_t *post_send_body = (uint8_t *)malloc(sizeof(uint8_t) * 1024);
                uint8_t *post_send_header = (uint8_t *)malloc(sizeof(uint8_t) * 512);

                uint8_t *boundary = "----WebKitFormBoundaryO2aA3WiAfUqIcD6e"; //header 中要比正文少俩--
                                                                              //结尾又比正文在街位数多俩--
                sprintf(post_send_header, "Sec-Fetch-Mode: cors\r\nUser-Agent: Xel\r\nContent-Type: multipart/form-data; boundary=%s\r\n", boundary);
                sprintf(post_send_body, "--%s\r\nContent-Disposition: form-data; name=\"file\"\r\n\r\nmultipart\r\n--%s\r\nContent-Disposition: form-data; name=\"Filedata\"; filename=\"logo.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n", boundary, boundary);

                tm = sysctl_get_time_us();
                uint32_t post_recv_len = http_post_file("http://api.uomg.com/api/image.ali",
                                                        post_send_header,
                                                        post_send_body,
                                                        boundary,
                                                        display_image,
                                                        get_recv_len,
                                                        http_header,
                                                        sizeof(http_header),
                                                        display_image,
                                                        sizeof(display_image));

                tt = sysctl_get_time_us() - tm;

                if (post_recv_len > 0)
                {
                    printk("post_recv_len %d \r\n", post_recv_len);
                    printk("http_header:%s\r\n", http_header);
                    printf("http_body:\r\n%s\r\n", display_image);

                    time_s = (float)((float)tt / 1000.0 / 1000.0);
                    file_kb = (float)((float)get_recv_len / 1024.0);

                    printk("http post use %ld us\r\n", tt);

                    printf("about %f KB/s\r\n", (float)(file_kb / time_s));
                }
#endif
            }
            while (1)
                ;
        }

        printf("should config wifi ssid and passwd \r\n");

        if (!qrcode_get_info_flag)
        {
            memset(display_image, 0, sizeof(display_image));
            image_rgb565_draw_string(display_image, "TOUCH KEY TO CONFIG WiFi...", 16, 40, 0, RED, NULL, 320, 240);
#if (CONFIG_LCD_WIDTH == 240)
            convert_320x240_to_240x240(display_image, 40);
            lcd_draw_picture(0, 0, 240, 240, (uint32_t *)display_image);
#else
            lcd_draw_picture(0, 0, 320, 240, (uint32_t *)display_image);
#endif
        }
#else

        /* if rcv jpg or scan qrcode, will stuck a period */
#if CONFIG_WIFI_ENABLE
        if (!jpeg_recv_start_flag && !qrcode_get_info_flag)
        {
            face_recognition_cfg.auto_out_fea = (uint8_t)g_board_cfg.brd_soft_cfg.cfg.auto_out_fea;
            face_recognition_cfg.compare_threshold = (float)g_board_cfg.brd_soft_cfg.out_threshold;
            face_lib_run(&face_recognition_cfg);
        }
#else
        if (!jpeg_recv_start_flag)
        {
            face_recognition_cfg.auto_out_fea = (uint8_t)g_board_cfg.brd_soft_cfg.cfg.auto_out_fea;
            face_recognition_cfg.compare_threshold = (float)g_board_cfg.brd_soft_cfg.out_threshold;
            face_lib_run(&face_recognition_cfg);
        }
#endif

#endif
        /* get key state */
        update_key_state();

#if CONFIG_KEY_SHORT_QRCODE
        /* key short to scan qrcode */
        if (g_key_press)
        {
            g_key_press = 0;
            qrcode_get_info_flag = 1;
            qrcode_start_time = sysctl_get_time_us();

            set_IR_LED(0);
            open_gc0328_650();
            dvp_set_output_enable(0, 0); //disable to ai
            dvp_set_output_enable(1, 1); //enable to lcd
        }

        if (qrcode_get_info_flag)
        {
            while (!g_dvp_finish_flag)
                ;

            qr_wifi_info_t *wifi_info = qrcode_get_wifi_cfg();
            if (NULL == wifi_info)
            {
                printk("no memory!\r\n");
            }
            else
            {
                switch (wifi_info->ret)
                {
                case QRCODE_RET_CODE_OK:
                {
                    printk("get qrcode\r\n");
                    printk("ssid:%s\tpasswd:%s\r\n", wifi_info->ssid, wifi_info->passwd);

                    if (g_net_status)
                    {
                        printk("already connect net, but want to reconfig, so reboot 8266\r\n");
                        spi_8266_init_device();
                        g_net_status = 0;
                    }

                    //connect to network
                    lcd_display_image_alpha(IMG_CONNING_ADDR, 0);
                    g_net_status = spi_8266_connect_ap(wifi_info->ssid, wifi_info->passwd, 2);
                    if (g_net_status)
                    {
                        lcd_display_image_alpha(IMG_CONN_SUCC_ADDR, 0);
                        spi_8266_mqtt_init();
                        memcpy(g_board_cfg.wifi_ssid, wifi_info->ssid, 32);
                        memcpy(g_board_cfg.wifi_passwd, wifi_info->passwd, 32);
                        if (flash_save_cfg(&g_board_cfg) == 0)
                        {
                            printk("save g_board_cfg failed!\r\n");
                        }
                    }
                    else
                    {
                        lcd_display_image_alpha(IMG_CONN_FAILED_ADDR, 0);
                        printk("wifi config maybe error,we can not connect to it!\r\n");
                    }
                    qrcode_get_info_flag = 0;
                }
                break;
                case QRCODE_RET_CODE_PRASE_ERR:
                {
                    printk("get error qrcode\r\n");
                }
                break;
                case QRCODE_RET_CODE_TIMEOUT:
                {
                    printk("scan qrcode timeout\r\n");
                    /* here display pic */
                    lcd_display_image_alpha(IMG_QR_TIMEOUT_ADDR, 0);
                    qrcode_get_info_flag = 0;
                }
                break;
                case QRCODE_RET_CODE_NO_DATA:
                default:
                    break;
                }
                free(wifi_info);
            }

            /* here display pic */
            lcd_display_image_alpha(IMG_SCAN_QR_ADDR, 160);

            g_dvp_finish_flag = 0;
        }
#endif /* CONFIG_KEY_SHORT_QRCODE */

        if (g_key_long_press)
        {
            g_key_long_press = 0;
            /* key long press */
            printk("key long press\r\n");

#if CONFIG_LONG_PRESS_FUNCTION_KEY_RESTORE
            /* set cfg to default */
            printk("reset board config\r\n");
            board_cfg_t board_cfg;

            memset(&board_cfg, 0, sizeof(board_cfg_t));

            flash_cfg_set_default(&board_cfg);

            if (flash_save_cfg(&board_cfg) == 0)
            {
                printk("save board_cfg failed!\r\n");
            }
            /* set cfg to default end */
#if CONFIG_LONG_PRESS_FUNCTION_KEY_CLEAR_FEATURE
            printk("Del All feature!\n");
            flash_delete_face_all();
#endif
            char *str_del = (char *)malloc(sizeof(char) * 32);
            sprintf(str_del, "Factory Reset...");
            if (lcd_dis_list_add_str(1, 1, 16, 0, str_del, 40, CONFIG_LCD_WIDTH - 16, RED, 1) == NULL)
            {
                printf("add dis str failed\r\n");
            }
            delay_flag = 1;
#endif
        }

        tim = sysctl_get_time_us();

#if CONFIG_NET_DEMO_MQTT
        /******Process mqtt ********/
        if (g_net_status && !qrcode_get_info_flag)
        {
            /* mqtt loop */
            if (PubSubClient_loop() == false)
            { /* disconnect */
                mqtt_reconnect();
            }

            if (last_mqtt_check_tim < tim)
            {
                if (!PubSubClient_connected())
                    mqtt_reconnect();
                last_mqtt_check_tim += 1000 * 1000; //1000ms
            }
        }
#endif /* CONFIG_NET_DEMO_MQTT */

        /******Process relay open********/
        tim = sysctl_get_time_us();
        tim = tim / 1000 / 1000;

        if (relay_open_flag && ((tim - last_open_relay_time_in_s) >= g_board_cfg.brd_soft_cfg.cfg.relay_open_s))
        {
            close_relay();
            relay_open_flag = 0;
        }

#if (CONFIG_PROTO_OVER_NET == 0)
        /******Process uart protocol********/
        if (recv_over_flag)
        {
            protocol_prase(cJSON_prase_buf);
            recv_over_flag = 0;
        }

        if (jpeg_recv_start_flag)
        {
            tim = sysctl_get_time_us();
            if (tim - jpeg_recv_start_time >= 10 * 1000 * 1000) //FIXME: 10s or 5s timeout
            {
                printk("abort to recv jpeg file\r\n");
                jpeg_recv_start_flag = 0;
                protocol_stop_recv_jpeg();
                protocol_send_cal_pic_result(7, "timeout to recv jpeg file", NULL, NULL, 0); //7  jpeg verify error
            }

            /* recv over */
            if (jpeg_recv_len != 0)
            {
                protocol_cal_pic_fea(&cal_pic_cfg, protocol_send_cal_pic_result);
                jpeg_recv_len = 0;
                jpeg_recv_start_flag = 0;
            }
        }
#else
        jpeg_recv_start_flag = 0;
        protocol_stop_recv_jpeg();
        protocol_send_cal_pic_result(2, "not support cal pic fea over net", NULL, NULL, 0); //7  jpeg verify error
#endif
    }
}
