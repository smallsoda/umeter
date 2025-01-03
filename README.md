# umeter
umeter, FreeRTOS, SIM800L

### Connection diagram
```
╔═════════════════╗                               ╔═════════════════╗
║ SIM800L         ║                               ║ STM32           ║
║ (module)    NET ╟─                              ║ (module)        ║
║             VCC ╟─> VBAT                        ║                 ║
║             RST ╟────────────[ 1.5k ]───────────╢ PB9             ║
║             RXD ╟────────────[ 1.5k ]───────────╢ PA2 (TX)        ║
║             TXD ╟────────────[ 1.5k ]───────────╢ PA3 (RX)        ║
║             GND ╟─> GND                         ║                 ║
║                 ║                               ║                 ║
║            SPK- ╟─                              ║                 ║
║            SPK+ ╟─                              ║                 ║
║            MIC- ╟─                              ║                 ║
║            MIC+ ╟─                              ║                 ║
║             DTR ╟─                              ║                 ║
║            RING ╟─                              ║                 ║
╚═════════════════╝                               ║                 ║
                                                  ╟─────────────────╢
╔═════════════════╗                               ║                 ║
║ W25Q            ║                               ║                 ║
║             /CS ╟───────────────────────────────╢ PB12            ║
║              DO ╟───────────────────────────────╢ PB14            ║
║             /WP ╟─> 3V3                         ║                 ║
║             GND ╟─> GND                         ║                 ║
║                 ║                               ║                 ║
║              DI ╟───────────────────────────────╢ PB15            ║
║             CLK ╟───────────────────────────────╢ PB13            ║
║          /RESET ╟─┐                             ║                 ║
║             VCC ╟─┴─> 3V3                       ║                 ║
╚═════════════════╝                               ║                 ║
                                                  ╟─────────────────╢
╔═════════════════╗                               ║                 ║
║ TMP75           ║                               ║                 ║
║             SDA ╟───────┬────────────────┬──────╢ PB11            ║
║             SCL ╟───────│─┬──────────────│─┬────╢ PB10            ║
║           ALERT ╟─      │ └──[ 10k ]──┐  │ │    ║                 ║
║             GND ╟─> GND └────[ 10k ]──┤  │ │    ║                 ║
║                 ║                     │  │ │    ║                 ║
║              A2 ╟─────────────────────┤  │ │    ║                 ║
║              A1 ╟─────────────────────┤  │ │    ║                 ║
║              A0 ╟─────────────────────┤  │ │    ║                 ║
║              V+ ╟─────────────────────┴──│─│─┬──╢ PB1             ║
╚═════════════════╝                        │ │ │  ║                 ║
                                           │ │ │  ╟─ ─ ─ ─ ─ ─ ─ ─ ─╢
╔═════════════════╗                        │ │ │  ║                 ║
║ AHT20           ║                        │ │ │  ║                 ║
║ (module)    SDA ╟────────────────────────┘ │ │  ║                 ║
║             SCL ╟──────────────────────────┘ │  ║                 ║
║             VIN ╟────────────────────────────┘  ║                 ║
║             GND ╟─> GND                         ║                 ║
╚═════════════════╝                               ║                 ║
                                                  ╟─────────────────╢
╔═════════════════╗                               ║                 ║
║ OPTOCOUPLER     ║                               ║                 ║
║ (module)     AO ╟─                              ║                 ║
║              DO ╟───────────────────────────────╢ PA0             ║
║             VCC ╟─────── ┐VT┌<─────────┬─> 3V3  ║                 ║
║             GND ╟─> GND  ╧══╧╕         │        ║                 ║
╚═════════════════╝            ├─[ 10k ]─┘        ║                 ║
                               └─[ 1k  ]──────────╢ PA1             ║
╔═════════════════╗                               ╟─────────────────╢
║ DEBUG UART      ║                               ║                 ║
║             TXD ╟───────────────────────────────╢ PA10 (RX)       ║
║             RXD ╟───────────────────────────────╢ PA9 (TX)        ║
║             GND ╟─> GND                         ║                 ║
╚═════════════════╝                               ║                 ║
                                                  ╟─────────────────╢
╔═════════════════╗                               ║                 ║
║ HMC5883L        ║                               ║                 ║
║ (module)    SDA ╟───────────────────────────────╢ PB7             ║
║             SCL ╟───────────────────────────────╢ PB6             ║
║            DRDY ╟───────────────────────────────╢ PA6             ║
║             VCC ╟───────────────────────────────╢ PA7             ║
║             GND ╟─> GND                         ║                 ║
╚═════════════════╝                               ║                 ║
                                                  ╟─────────────────╢
                          VBAT <──[ 100k ? ]──┐   ║                 ║
                           GND <──[ 100k ? ]──┴───╢ PB0 (ADC)       ║
                                                  ║                 ║
                                                  ╟─────────────────╢
                                           VBAT <─╢ 5V              ║
                                            3V3 <─╢ 3V3 (out)       ║
                                            GND <─╢ GND             ║
                                                  ╚═════════════════╝
```
