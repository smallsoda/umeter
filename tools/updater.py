import serial
import time

CHECKSUM = 0x5A5A5A5A

CMD_START = 0x00
CMD_DATA = 0x01
CMD_RESET = 0x02

HEADER_SIZE = 4096
PACKET_PAYLOAD_SIZE = 16 * 4
PACKET_SIZE = 4 + 4 + PACKET_PAYLOAD_SIZE

COM = 'COM9'
# BIN = 'umeter.bin'
BIN = 'umeter-blink.bin'


def checksum(payload):
    sum = CHECKSUM
    i = 0
    while i < len(payload):
        sum += payload[i] | payload[i + 1] << 8 | payload[i + 2] << 16 | payload[i + 3] << 24
        i += 4
    sum &= 0xFFFFFFFF
    return sum.to_bytes(4, 'little')


def packet_start():
    cmd = CMD_START
    return cmd.to_bytes(4, 'little')


def packet_data(payload):
    cmd = CMD_DATA
    if len(payload) < PACKET_PAYLOAD_SIZE:
        payload = bytearray(payload)
        payload += bytearray([0xFF] * (PACKET_PAYLOAD_SIZE - len(payload)))
    packet = bytearray()
    packet += cmd.to_bytes(4, 'little')
    packet += checksum(payload)
    packet += payload

    return packet


def packet_reset():
    cmd = 0x02
    return cmd.to_bytes(4, 'little')


def fws(firmware):
    header = bytearray()

    loaded = 0
    version = 1
    size = len(firmware)
    cs = checksum(firmware)

    header += loaded.to_bytes(4, 'little')
    header += version.to_bytes(4, 'little')
    header += size.to_bytes(4, 'little')
    header += cs

    header += bytearray([0xFF] * (HEADER_SIZE - len(header)))
    return header


def send(com, packet):
    com.write(packet)
    ans = com.read(4)
    ret = int.from_bytes(ans, 'little', signed=True)
    if ret == -1:
        print('error')
        exit(1)


if __name__ == '__main__':
    com = serial.Serial(COM, 115200, timeout=0.1)

    send(com, packet_start())

    with open(BIN, 'rb') as file:
        fw = file.read()

    header = fws(fw)
    addr = 0
    while addr < len(header):
        send(com, packet_data(header[addr:addr + PACKET_PAYLOAD_SIZE]))
        addr += PACKET_PAYLOAD_SIZE

        if (addr & 0x0FFF) == 0:
            print('header', addr)

    addr = 0
    while addr < len(fw):
        send(com, packet_data(fw[addr:addr + PACKET_PAYLOAD_SIZE]))
        addr += PACKET_PAYLOAD_SIZE

        if (addr & 0x0FFF) == 0:
            print('firmware', addr)

    send(com, packet_reset())

    print('done')
