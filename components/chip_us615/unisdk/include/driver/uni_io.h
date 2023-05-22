/**
 * @file    uni_io.h
 *
 * @brief   IO Driver Module
 *
 *
 */
#ifndef UNI_IO_H
#define UNI_IO_H


#define TLS_IO_AB_OFFSET  (0x40011400 - 0x40011200)

/** io name */
enum tls_io_name {
    UNI_IO_PA_00 = 0,    /**< gpio a0 */
    UNI_IO_PA_01,        /**< gpio a1 */
    UNI_IO_PA_02,        /**< gpio a2 */
    UNI_IO_PA_03,        /**< gpio a3 */
    UNI_IO_PA_04,        /**< gpio a4 */
    UNI_IO_PA_05,        /**< gpio a5 */
    UNI_IO_PA_06,        /**< gpio a6 */
    UNI_IO_PA_07,        /**< gpio a7 */
    UNI_IO_PA_08,        /**< gpio a8 */
    UNI_IO_PA_09,        /**< gpio a9 */
    UNI_IO_PA_10,        /**< gpio a10 */
    UNI_IO_PA_11,        /**< gpio a11 */
    UNI_IO_PA_12,        /**< gpio a12 */
    UNI_IO_PA_13,        /**< gpio a13 */
    UNI_IO_PA_14,        /**< gpio a14 */
    UNI_IO_PA_15,        /**< gpio a15 */

    UNI_IO_PB_00,        /**< gpio b0 */
    UNI_IO_PB_01,        /**< gpio b1 */
    UNI_IO_PB_02,        /**< gpio b2 */
    UNI_IO_PB_03,        /**< gpio b3 */
    UNI_IO_PB_04,        /**< gpio b4 */
    UNI_IO_PB_05,        /**< gpio b5 */
    UNI_IO_PB_06,        /**< gpio b6 */
    UNI_IO_PB_07,        /**< gpio b7 */
    UNI_IO_PB_08,        /**< gpio b8 */
    UNI_IO_PB_09,        /**< gpio b9 */
    UNI_IO_PB_10,        /**< gpio b10 */
    UNI_IO_PB_11,        /**< gpio b11 */
    UNI_IO_PB_12,        /**< gpio b12 */
    UNI_IO_PB_13,        /**< gpio b13 */
    UNI_IO_PB_14,        /**< gpio b14 */
    UNI_IO_PB_15,        /**< gpio b15 */
    UNI_IO_PB_16,        /**< gpio b16 */
    UNI_IO_PB_17,        /**< gpio b17 */
    UNI_IO_PB_18,        /**< gpio b18 */
    UNI_IO_PB_19,        /**< gpio b19 */
    UNI_IO_PB_20,        /**< gpio b20 */
    UNI_IO_PB_21,        /**< gpio b21 */
    UNI_IO_PB_22,        /**< gpio b22 */
    UNI_IO_PB_23,        /**< gpio b23 */
    UNI_IO_PB_24,        /**< gpio b24 */
    UNI_IO_PB_25,        /**< gpio b25 */
    UNI_IO_PB_26,        /**< gpio b26 */
    UNI_IO_PB_27,        /**< gpio b27 */
    UNI_IO_PB_28,        /**< gpio b28 */
    UNI_IO_PB_29,        /**< gpio b29 */
    UNI_IO_PB_30,        /**< gpio b30 */
    UNI_IO_PB_31			/**< gpio b31 */
};

/** option 1 of the io */
#define UNI_IO_OPTION1               1
/** option 2 of the io */
#define UNI_IO_OPTION2               2
/** option 3 of the io */
#define UNI_IO_OPTION3               3
/** option 4 of the io */
#define UNI_IO_OPTION4               4
/** option 5 of the io */
#define UNI_IO_OPTION5               5
/** option 6 of the io */
#define UNI_IO_OPTION6               6


/* io option1 */
#define UNI_IO_OPT1_I2C_DAT          UNI_IO_OPTION1
#define UNI_IO_OPT1_PWM1             UNI_IO_OPTION1
#define UNI_IO_OPT1_PWM2             UNI_IO_OPTION1
#define UNI_IO_OPT1_PWM3             UNI_IO_OPTION1
#define UNI_IO_OPT1_PWM4             UNI_IO_OPTION1
#define UNI_IO_OPT1_PWM5             UNI_IO_OPTION1
#define UNI_IO_OPT1_UART0_RXD        UNI_IO_OPTION1
#define UNI_IO_OPT1_UART0_TXD        UNI_IO_OPTION1
#define UNI_IO_OPT1_PWM_BRAKE        UNI_IO_OPTION1
#define UNI_IO_OPT1_I2S_M_EXTCLK     UNI_IO_OPTION1
#define UNI_IO_OPT1_SPI_M_DO         UNI_IO_OPTION1
#define UNI_IO_OPT1_SPI_M_DI         UNI_IO_OPTION1
#define UNI_IO_OPT1_SPI_M_CS         UNI_IO_OPTION1
#define UNI_IO_OPT1_SPI_M_CK         UNI_IO_OPTION1
#define UNI_IO_OPT1_I2S_S_RL         UNI_IO_OPTION1
#define UNI_IO_OPT1_I2S_S_SCL        UNI_IO_OPTION1
#define UNI_IO_OPT1_I2S_S_SDA        UNI_IO_OPTION1
#define UNI_IO_OPT1_I2S_M_RL         UNI_IO_OPTION1
#define UNI_IO_OPT1_I2S_M_SCL        UNI_IO_OPTION1
#define UNI_IO_OPT1_I2S_M_SDA        UNI_IO_OPTION1
#define UNI_IO_OPT1_JTAG_RST         UNI_IO_OPTION1
#define UNI_IO_OPT1_JTAG_TDO         UNI_IO_OPTION1
#define UNI_IO_OPT1_JTAG_TDI         UNI_IO_OPTION1
#define UNI_IO_OPT1_JTAG_TCK_SWDCK   UNI_IO_OPTION1
#define UNI_IO_OPT1_JTAG_TMS_SWDAT   UNI_IO_OPTION1
#define UNI_IO_OPT1_UART1_RXD        UNI_IO_OPTION1
#define UNI_IO_OPT1_UART1_TXD        UNI_IO_OPTION1
#define UNI_IO_OPT1_UART1_RTS        UNI_IO_OPTION1
#define UNI_IO_OPT1_UART1_CTS        UNI_IO_OPTION1
#define UNI_IO_OPT1_SDIO_DAT         UNI_IO_OPTION1

/* io option2 */
#define UNI_IO_OPT2_PWM1             UNI_IO_OPTION2
#define UNI_IO_OPT2_PWM2             UNI_IO_OPTION2
#define UNI_IO_OPT2_PWM3             UNI_IO_OPTION2
#define UNI_IO_OPT2_PWM4             UNI_IO_OPTION2
#define UNI_IO_OPT2_PWM5             UNI_IO_OPTION2
#define UNI_IO_OPT2_SPI_M_DO         UNI_IO_OPTION2
#define UNI_IO_OPT2_SPI_M_DI         UNI_IO_OPTION2
#define UNI_IO_OPT2_SPI_M_CS         UNI_IO_OPTION2
#define UNI_IO_OPT2_SPI_M_CK         UNI_IO_OPTION2
#define UNI_IO_OPT2_I2C_SCL          UNI_IO_OPTION2
#define UNI_IO_OPT2_I2S_M_EXTCLK     UNI_IO_OPTION2
#define UNI_IO_OPT2_UART1_RXD        UNI_IO_OPTION2
#define UNI_IO_OPT2_UART1_TXD        UNI_IO_OPTION2
#define UNI_IO_OPT2_UART1_RTS        UNI_IO_OPTION2
#define UNI_IO_OPT2_UART1_CTS        UNI_IO_OPTION2
#define UNI_IO_OPT2_I2C_DAT          UNI_IO_OPTION2
#define UNI_IO_OPT2_PWM_BRAKE        UNI_IO_OPTION2
#define UNI_IO_OPT2_UART0_RTS        UNI_IO_OPTION2
#define UNI_IO_OPT2_UART0_CTS        UNI_IO_OPTION2
#define UNI_IO_OPT2_SDIO_DAT         UNI_IO_OPTION2
#define UNI_IO_OPT2_HSPI_CK          UNI_IO_OPTION2
#define UNI_IO_OPT2_HSPI_INT         UNI_IO_OPTION2
#define UNI_IO_OPT2_HSPI_CS          UNI_IO_OPTION2
#define UNI_IO_OPT2_HSPI_DI          UNI_IO_OPTION2
#define UNI_IO_OPT2_HSPI_DO          UNI_IO_OPTION2

/* io option3 */
#define UNI_IO_OPT3_UART0_RXD        UNI_IO_OPTION3
#define UNI_IO_OPT3_UART0_TXD        UNI_IO_OPTION3
#define UNI_IO_OPT3_UART0_RTS        UNI_IO_OPTION3
#define UNI_IO_OPT3_UART0_CTS        UNI_IO_OPTION3
#define UNI_IO_OPT3_SPI_M_DO         UNI_IO_OPTION3
#define UNI_IO_OPT3_SPI_M_DI         UNI_IO_OPTION3
#define UNI_IO_OPT3_SPI_M_CS         UNI_IO_OPTION3
#define UNI_IO_OPT3_SDIO_CK          UNI_IO_OPTION3
#define UNI_IO_OPT3_SDIO_CMD         UNI_IO_OPTION3
#define UNI_IO_OPT3_SDIO_DAT         UNI_IO_OPTION3

/* io option4 */
#define UNI_IO_OPT4_I2S_M_MCLK       UNI_IO_OPTION4
#define UNI_IO_OPT4_I2S_M_RL         UNI_IO_OPTION4
#define UNI_IO_OPT4_I2S_M_SCL        UNI_IO_OPTION4
#define UNI_IO_OPT4_I2S_M_SDA        UNI_IO_OPTION4

/* io option5 */
#define UNI_IO_OPT5_GPIO             UNI_IO_OPTION5

/* io option6 */
#define UNI_IO_OPT6_ADC              UNI_IO_OPTION6
#define UNI_IO_OPT6_LCD_COM          UNI_IO_OPTION6
#define UNI_IO_OPT6_LCD_SEG          UNI_IO_OPTION6


/**
 * @brief          	This function is used to config io function
 *
 * @param[in]      	name      io name
 * @param[in]      	option    io function option, value is UNI_IO_OPT*_*, also is UNI_IO_OPTION1~6
 *
 * @return         	None
 *
 * @note           	None
 */
void tls_io_cfg_set(enum tls_io_name name, uint8_t option);


/**
 * @brief          	This function is used to get io function config 
 *
 * @param[in]      	name      io name
 *
 * @retval         	UNI_IO_OPTION1~6  Mapping io function
 *
 * @note           	None
 */
int tls_io_cfg_get(enum tls_io_name name);


#endif  /* end of UNI_IO_H */

