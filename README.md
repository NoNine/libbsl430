libbsl430
---------
A MSP430 BSL protocol ('5xx, 6xx' UART) host side implementation.

It is verified with MSP430FR2x33 and the UART is used as the<br />
communication interface.

SLAU610A: MSP430FR4xx and MSP430FR2xx Bootloader (BSL)<br />
SLAU319K: MSP430 Programming With the Bootloader (BSL)


Code Structure
--------------
```
libbsl430/
+-- Android.mk           Makefile following Android build system.
+-- bsl430.c             BSL protocol core commands implementation.
+-- bsl430.h
+-- bsl430-platform.c    Platform specific code for GPIO/UART access.
+-- bsl430-platform.h
+-- bsl430-program.c     BSL protocol programing process implementation.
+-- bsl430-program.h
+-- bsl430_test.c        The test code parses a TI-TXT file and programs it.
+-- README
```


How to Port the library
-----------------------
All the platform specific codes are in bellow two files.<br />
You can create your own ones.

    bsl430-platform.h
    bsl430-platform.c

Below Macros and APIs are needed to be implemented to support the library.

    log(...)
    debug(...)
    mdelay(a)

    int bsl430_uart_init(int baudrate, int parity);
    int bsl430_uart_term(void);
    int bsl430_uart_readb(uint16_t timeout);
    int bsl430_uart_writeb(uint8_t c);
    int bsl430_uart_clear(void);

    int bsl430_gpio_init(void);
    int bsl430_gpio_term(void);
    int bsl430_gpio_rst(int level);
    int bsl430_gpio_tst(int level);


How to Run the Test
-------------------
1) Port the library to your platform and pass the build.<br />
2) bsl430_test can be run in below form.

    $ bsl430_test <TI-TXT File>

    Below is an example console output which shows the programing process.

    bsl430_test: File size: 45216 Byte.
    bsl430: Change baudrate to 115200.
    bsl430-program: ** Password Error! All code FRAM is erased!
    bsl430-program: BSL Version: 000835B3
    bsl430-program: <<< Segment: @C400 285 Bytes, Crc C49C >>>
    bsl430: RX_DATA: @C400 256 Bytes
    bsl430: RX_DATA: @C500  29 Bytes
    bsl430-program:
    bsl430-program: <<< Segment: @C51E 14448 Bytes, Crc BBDC >>>
    bsl430: RX_DATA: @C51E 256 Bytes
    bsl430: RX_DATA: @C61E 256 Bytes
    bsl430: RX_DATA: @C71E 256 Bytes
    bsl430: RX_DATA: @C81E 256 Bytes
    bsl430: RX_DATA: @C91E 256 Bytes
    bsl430: RX_DATA: @CA1E 256 Bytes
    bsl430: RX_DATA: @CB1E 256 Bytes
    bsl430: RX_DATA: @CC1E 256 Bytes
    bsl430: RX_DATA: @CD1E 256 Bytes
    bsl430: RX_DATA: @CE1E 256 Bytes
    bsl430: RX_DATA: @CF1E 256 Bytes
    bsl430: RX_DATA: @D01E 256 Bytes
    bsl430: RX_DATA: @D11E 256 Bytes
    bsl430: RX_DATA: @D21E 256 Bytes
    bsl430: RX_DATA: @D31E 256 Bytes
    bsl430: RX_DATA: @D41E 256 Bytes
    bsl430: RX_DATA: @D51E 256 Bytes
    bsl430: RX_DATA: @D61E 256 Bytes
    bsl430: RX_DATA: @D71E 256 Bytes
    bsl430: RX_DATA: @D81E 256 Bytes
    bsl430: RX_DATA: @D91E 256 Bytes
    bsl430: RX_DATA: @DA1E 256 Bytes
    bsl430: RX_DATA: @DB1E 256 Bytes
    bsl430: RX_DATA: @DC1E 256 Bytes
    bsl430: RX_DATA: @DD1E 256 Bytes
    bsl430: RX_DATA: @DE1E 256 Bytes
    bsl430: RX_DATA: @DF1E 256 Bytes
    bsl430: RX_DATA: @E01E 256 Bytes
    bsl430: RX_DATA: @E11E 256 Bytes
    bsl430: RX_DATA: @E21E 256 Bytes
    bsl430: RX_DATA: @E31E 256 Bytes
    bsl430: RX_DATA: @E41E 256 Bytes
    bsl430: RX_DATA: @E51E 256 Bytes
    bsl430: RX_DATA: @E61E 256 Bytes
    bsl430: RX_DATA: @E71E 256 Bytes
    bsl430: RX_DATA: @E81E 256 Bytes
    bsl430: RX_DATA: @E91E 256 Bytes
    bsl430: RX_DATA: @EA1E 256 Bytes
    bsl430: RX_DATA: @EB1E 256 Bytes
    bsl430: RX_DATA: @EC1E 256 Bytes
    bsl430: RX_DATA: @ED1E 256 Bytes
    bsl430: RX_DATA: @EE1E 256 Bytes
    bsl430: RX_DATA: @EF1E 256 Bytes
    bsl430: RX_DATA: @F01E 256 Bytes
    bsl430: RX_DATA: @F11E 256 Bytes
    bsl430: RX_DATA: @F21E 256 Bytes
    bsl430: RX_DATA: @F31E 256 Bytes
    bsl430: RX_DATA: @F41E 256 Bytes
    bsl430: RX_DATA: @F51E 256 Bytes
    bsl430: RX_DATA: @F61E 256 Bytes
    bsl430: RX_DATA: @F71E 256 Bytes
    bsl430: RX_DATA: @F81E 256 Bytes
    bsl430: RX_DATA: @F91E 256 Bytes
    bsl430: RX_DATA: @FA1E 256 Bytes
    bsl430: RX_DATA: @FB1E 256 Bytes
    bsl430: RX_DATA: @FC1E 256 Bytes
    bsl430: RX_DATA: @FD1E 112 Bytes
    bsl430-program:
    bsl430-program: <<< Segment: @FFD8 6 Bytes, Crc 87E4 >>>
    bsl430: RX_DATA: @FFD8   6 Bytes
    bsl430-program:
    bsl430-program: <<< Segment: @FFE2 4 Bytes, Crc 56CA >>>
    bsl430: RX_DATA: @FFE2   4 Bytes
    bsl430-program:
    bsl430-program: <<< Segment: @FFF2 2 Bytes, Crc 2300 >>>
    bsl430: RX_DATA: @FFF2   2 Bytes
    bsl430-program:
    bsl430-program: <<< Segment: @FFFE 2 Bytes, Crc 10BD >>>
    bsl430: RX_DATA: @FFFE   2 Bytes
    bsl430-program:
    bsl430-program: BSL programming SUCC.

