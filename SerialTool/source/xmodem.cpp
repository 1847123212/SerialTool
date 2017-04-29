#include "xmodem.h"

// ״̬��ʹ�õ��ַ�
#define NONE 0x00 // ����״̬
#define TRAN 0x01 // ����״̬

enum {
    None,         // ����״̬
    Trans,        // ����״̬
    TransEnd,     // �������״̬
    TransEOT      // EOT�ַ���Ӧ��״̬
};

#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

XModemClass::XModemClass(FileThread *thread)
{
    this->thread = thread;
}

void XModemClass::setThread(FileThread *thread)
{
    this->thread = thread;
}

// ����У��ֵ
char XModemClass::calcVerifi(const char *frame)
{
    int value = 0;

    // ֻʵ����У����㷨
    for (int i = 3; i < 128 + 3; ++i) {
        value += frame[i];
    }
    return (char)(value & 0xFF);
}

// ��ʼ����
void XModemClass::startTransmit()
{
    status = None;
    transMode = true;
}

// ��ʼ����
void XModemClass::startReceive()
{
    QByteArray arr;
    rxBuf.clear();
    lastCount = 0;
    transMode = false;

    arr.append(NAK);
    thread->sendPortData(arr);
}

// ȡ������
bool XModemClass::cancelTrans()
{
    QByteArray arr;

    if (transMode) { // ����ģʽ��ȡ������
        if (status == None) {
            arr.clear();
            arr.append(EOT);
            thread->sendPortData(arr);
        } else { // ����ģʽ��ȡ������
            arr.clear();
            arr.append(CAN);
            thread->sendPortData(arr);
        }
    }
    return true;
}

// ����ģʽÿ�ν���һ���ֽ�
int XModemClass::transmit(char ch, qint64 &bytes)
{
    int rBytes;
    QByteArray arr;

    if (ch == CAN) { // ȡ������
        return 1;
    }
    switch (status) {
    case None:
        if (ch == NAK) { // ��ʼ����
            status = ACK;
            memset(frame, 0, 132); // ��ʼ��֡
            ch = ACK; // Ϊ�����洫���һ֡
            status = Trans;
        }
    case TransEnd: // �������
        // ���յ�ACK˵�����һ֡���ݴ������, ��ʱ����EOT
        if (ch == ACK && status == TransEnd) {
            arr.clear();
            arr.append(EOT);
            thread->sendPortData(arr);
            status = TransEOT; // �������Ӧ��״̬
            break;
        }
    case Trans: // ��������
        if (ch == ACK) { // ������һ������
            frame[0] = SOH;
            ++frame[1];
            frame[2] = ~frame[1];
            rBytes = thread->readFile(frame + 3, 128);
            bytes += rBytes;
            if (rBytes < 128) { // ˵�������һ֡����
                // ������ֽ����0x1A
                for (int i = rBytes + 3; i < 131; ++i) {
                    frame[i] = 0x1A;
                }
                status = TransEnd;
            }
            frame[131] = calcVerifi(frame); // ����У���ַ�
        }
        arr.clear();
        arr.append(frame, 132);
        thread->sendPortData(arr); // ��������
        break;
    case TransEOT:
        if (ch == ACK) { // EOT���Է�Ӧ��, �������
            status = None;
            return 1;
        }
        break;
    }
    return 0;
}

// ����ģʽ
int XModemClass::receive(const QByteArray &arr, qint64 &bytes)
{
    int size, pos;
    const char *pdata;
    QByteArray tbuf;

    rxBuf.append(arr);
    size = rxBuf.size();
    pdata = rxBuf.data();
    if (pdata[0] == EOT) { // �������
        tbuf.append(ACK);
        thread->sendPortData(tbuf); // ����Ӧ���־
        return 1;
    }
    // Ѱ��SOH�ַ�
    for (pos = 0; pos < size && pdata[pos] != SOH; ++pos);
    if (size - pos < 131) { // ���������峤��С��132bytes˵������������֡
        return 0;
    }
    pdata += pos; // ��ȡ֡����
    // У��
    qDebug("last: %d, pdata[1]: %d", lastCount, pdata[1]);
    if (calcVerifi(pdata) == pdata[131] // ���У���ֽ�
        && pdata[1] == ~pdata[2] // ֡����У��
        && ((char)(lastCount + 1) == pdata[1] || lastCount == pdata[1])) { // ֡���Ƿ�����
        thread->writeFile(pdata + 3, 128);
        bytes += 128;
        lastCount = pdata[1];
        tbuf.append(ACK);
        thread->sendPortData(tbuf); // ����Ӧ���־
    } else {
        tbuf.append(NAK);
        thread->sendPortData(tbuf); // ����Ӧ���־
    }
    rxBuf = rxBuf.mid(pos + 132); // ������������
    return 0;
}
