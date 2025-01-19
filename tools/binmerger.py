import argparse
import os

# Defaults
BOOT_MIN_SIZE = 1024 * 8
BOOT = 'bootloader.bin'
APP = 'app.bin'


def copy_bootloader(boot_min_size, boot_name, out_name):
    out_size = 0

    with open(out_name, 'wb') as out:
        with open(boot_name, 'rb') as boot:
            block = boot.read()
            out_size += out.write(block)
            print('boot: {}'.format(out_size))
        if out_size < boot_min_size:
            block = bytearray([0xFF] * (boot_min_size - out_size))
            out_size += out.write(block)
            print('boot empty: {}'.format(len(block)))

    return out_size
        

def copy_application(app_name, out_name):
    with open(out_name, 'ab') as out:
        with open(app_name, 'rb') as app:
            block = app.read()
            out.write(block)
            print('app: {}'.format(len(block)))

    return len(block)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Merge bootloader with application')
    parser.add_argument('-b', '--boot', type=str, help='bootloader binary')
    parser.add_argument('-a', '--app', type=str, help='application binary')
    parser.add_argument('-s', '--size', type=int, help='bootloader minimal size')
    args = parser.parse_args()

    if args.boot is None:
        args.boot = BOOT
    if args.app is None:
        args.app = APP
    if args.size is None:
        args.size = BOOT_MIN_SIZE

    fboot = os.path.basename(args.boot).split('/')[-1]
    fapp = os.path.basename(args.app).split('/')[-1]
    #fout = fboot.replace('.bin', '') + '-' + fapp
    fout = 'bl-' + fapp

    print('bootloader: {}'.format(args.boot))
    print('application: {}'.format(args.app))

    sboot = copy_bootloader(args.size, args.boot, fout)
    sapp = copy_application(args.app, fout)
    print('boot+app: {}'.format(sboot + sapp))
