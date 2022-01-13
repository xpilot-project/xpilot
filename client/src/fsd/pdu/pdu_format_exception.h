#ifndef PDU_FORMAT_EXCEPTION_H
#define PDU_FORMAT_EXCEPTION_H

#include <QString>
#include <QException>

class PDUFormatException : public QException
{
public:
    PDUFormatException(QString const &error, QString const &rawMessage) : error(error), rawMessage(rawMessage) {}
    virtual ~PDUFormatException() {}

    void raise() const override { throw *this; }
    PDUFormatException * clone() const override { return new PDUFormatException(*this); }

    QString getError() const { return error; }
    QString getRawMessage() const { return rawMessage; }
private:
    QString error;
    QString rawMessage;
};

#endif // PDU_FORMAT_EXCEPTION_H
