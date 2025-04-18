# umeter
umeter, FreeRTOS, SIM800L

### Connection diagram
```
╔═════════════════╗                                    ╔═════════════════╗
║ SIM800L         ║                                    ║ STM32           ║
║ (module)    NET ╟─                                   ║ (module)        ║
║             VCC ╟─> VBAT                             ║                 ║
║             RST ╟────────────────────────────────────╢ PB9             ║
║             RXD ╟──────────────[ 1.5k ]──────────────╢ PA2 (TX)        ║
║             TXD ╟──────────────[ 1.5k ]──────────────╢ PA3 (RX)        ║
║             GND ╟─> GND                              ║                 ║
║                 ║                                    ║                 ║
║            SPK- ╟─                                   ║                 ║
║            SPK+ ╟─                                   ║                 ║
║            MIC- ╟─                                   ║                 ║
║            MIC+ ╟─                                   ║                 ║
║             DTR ╟─                                   ║                 ║
║            RING ╟─                                   ║                 ║
╚═════════════════╝                                    ║                 ║
                                                       ╟─────────────────╢
╔═════════════════╗                                    ║                 ║
║ W25Q            ║                                    ║                 ║
║             /CS ╟────────────────────────────────────╢ PB12            ║
║              DO ╟────────────────────────────────────╢ PB14            ║
║             /WP ╟─> 3V3                              ║                 ║
║             GND ╟─> GND                              ║                 ║
║                 ║                                    ║                 ║
║              DI ╟────────────────────────────────────╢ PB15            ║
║             CLK ╟────────────────────────────────────╢ PB13            ║
║          /RESET ╟─┐                                  ║                 ║
║             VCC ╟─┴─> 3V3                            ║                 ║
╚═════════════════╝                                    ║                 ║
                                                       ╟─────────────────╢
╔═════════════════╗                                    ║                 ║
║ TMP75           ║                                    ║                 ║
║             SDA ╟───────┬──────────────  x──┬────────╢ PB11            ║
║             SCL ╟───────│─┬────────────  x──│─┬──────╢ PB10            ║
║           ALERT ╟─      │ └──[ 10k ]──┐     │ │      ║                 ║
║             GND ╟─> GND └────[ 10k ]──┤     │ │      ║                 ║
║                 ║                     │     │ │      ║                 ║
║              A2 ╟─────────────────────┤     │ │      ║                 ║
║              A1 ╟─────────────────────┤     │ │      ║                 ║
║              A0 ╟─────────────────────┤     │ │      ║                 ║
║              V+ ╟─────────────────────┴─────│─│─  x──╢ PA6             ║
╚═════════════════╝                           │ │      ║                 ║
                                              │ │      ╟─ ─ ─ ─ ─ ─ ─ ─ ─╢
╔═════════════════╗                           │ │      ║                 ║
║ AHT20           ║                           │ │      ║                 ║
║ (module)    SDA ╟───────────────────────────┘ │      ║                 ║
║             SCL ╟─────────────────────────────┘      ║                 ║
║             VIN ╟─────── ┐VT┌<──────────┬───> 3V3    ║                 ║
║             GND ╟─> GND  ╧══╧╕          │            ║                 ║
╚═════════════════╝            ├─[ 10k ]──┤            ║                 ║
                               └─[ 1k  ]──│────────────╢ PB1             ║
                                          │  ││        ║                 ║
                                          └──┤├──> GND ║                 ║
                                             ││22u     ║                 ║
╔═════════════════╗                                    ╟─────────────────╢
║ OPTOCOUPLER     ║                                    ║                 ║
║ (module)     AO ╟────────────────────────────────────╢ PA0             ║
║              DO ╟─                                   ║                 ║
║             VCC ╟─────── ┐VT┌<─────────┬─> 3V3       ║                 ║
║             GND ╟─> GND  ╧══╧╕         │             ║                 ║
╚═════════════════╝            ├─[ 10k ]─┘             ║                 ║
                               └─[ 1k  ]───────────────╢ PA1             ║
╔═════════════════╗                                    ╟─────────────────╢
║ DEBUG UART      ║                                    ║                 ║
║             TXD ╟────────────────────────────────────╢ PA10 (RX)       ║
║             RXD ╟────────────────────────────────────╢ PA9 (TX)        ║
║             GND ╟─> GND                              ║                 ║
╚═════════════════╝                                    ║                 ║
                                                       ╟─────────────────╢
╔═════════════════╗                                    ║                 ║
║ HMC5883L        ║                                    ║                 ║
║ (module)    SDA ╟───────────────────────────────  x──╢ PB7             ║
║             SCL ╟───────────────────────────────  x──╢ PB6             ║
║            DRDY ╟───────────────────────────────  x──╢ PA6             ║
║             VCC ╟───────────────────────────────  x──╢ PA7             ║
║             GND ╟─> GND                              ║                 ║
╚═════════════════╝                                    ║                 ║
                                                       ╟─────────────────╢
                                   External WATCHDOG <─╢ PB3             ║
                                                       ╟─────────────────╢
                                              TAMPER <─╢ PA15            ║
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
