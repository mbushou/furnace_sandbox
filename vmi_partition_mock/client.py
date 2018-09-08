#!/usr/bin/python3
import vmi_pb2
import zmq
import time
import sys

def getmsg(msgtype):
    pl = vmi_pb2.VMIMessage()
    #msgtype = 'READ_8_VA'
    pl.Clear()
    msgnum = pl.__getattribute__(msgtype)
    pl.type.append(msgnum)
    submsg = getattr(pl, msgtype.lower()).add()
    submsg.vaddr = 0x1234
    submsg.pid = 0
    return pl

def main():

    context = zmq.Context()

    socket_string = 'ipc:///tmp/test_vmi_runtime'

    dealer = context.socket(zmq.DEALER)
    dealer.identity = 'testclientd'.encode('ascii')
    dealer.connect(socket_string)

    req = context.socket(zmq.REQ)
    req.identity = 'testclientr'.encode('ascii')
    req.connect(socket_string)

    poller = zmq.Poller()
    poller.register(dealer, zmq.POLLIN)


    msg = vmi_pb2.VMIMessage()

    pl = getmsg('READ_8_VA')
    print(f'{pl}')
    req.send(pl.SerializeToString())
    print(f'ok! {req.recv()}')

    print(f'done!')

    i = 0
    while True:

        socks = dict(poller.poll(100))

        if dealer in socks:
            raw_msg = dealer.recv()
            printf(f'in dealer: {raw_msg}')
            #msg.ParseFromString(raw_msg)
            #printf(f'{msg}')

            printf(f'out req: {pl}')
            req.send(pl.SerializeToString())
            raw_msg = req.recv()
            #msg.ParseFromString(req.recv())
            printf(f'in req: {raw_msg}')
        else:
            sys.stdout.write('.')
            sys.stdout.flush()

        if not (i%5):
            sys.stdout.write(f'{i}')
            if not (i%20):
                req.send(getmsg('READ_64_VA').SerializeToString())
            elif not (i%15):
                req.send(getmsg('READ_32_VA').SerializeToString())
            elif not (i%10):
                req.send(getmsg('READ_16_VA').SerializeToString())
            else:
                req.send(getmsg('READ_8_VA').SerializeToString())
            print(f'ok! {req.recv()}')

        i += 1

if __name__ == '__main__':
    s = time.time()
    try:
        main()
    except KeyboardInterrupt:
        pass
