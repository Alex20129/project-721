#ifndef ASICDEVICE_H
#define ASICDEVICE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QHostAddress>
#include <QtNetwork>
#include <QApplication>

class Pool
{
public:
    QUrl URL;
    uint priority;
    bool StratumActive;
    QString User;
    QString Status;
    Pool();
};

class ASICDevice : public QObject
{
    Q_OBJECT
signals:
    void HaveAPendingCommands();
    void Updated(ASICDevice *device);
    void SocketConnected(ASICDevice *device);
    void SocketError(ASICDevice *device);
    void Alarm(uint alarmCode);
public:
    uint GroupID;
    static unsigned int ActiveThreadsNum;
    enum AlarmFlags
    {
		NoAlarm				=0x0000,
		AlarmThermalRunout	=0x0001,
		AlarmHashrateDecline=0x0002,
		AlarmSocketTimeout	=0x0004,
		AlarmSocketError	=0x0008
    };
    quint16 WebPort;
    quint16 APIPort;
    QString HostName;
    QString UserName;
    QString CurrentOCProfile;
    QString Password;
    QHostAddress Address;
    QString Type;
    QString Miner;
    QString Description;
    bool IsAlarmed();
    uint State();
    Pool *ActivePool();
    uint ActiveChipsNumber();
    uint Frequency();
    uint HardwareErrors();
    double HashRate();
    double Temperature();
    uint UpTime();
    void ClearUpErrorFlags();
    explicit ASICDevice(QObject *parent=nullptr);
    ~ASICDevice();
public slots:
    void updateStats(QByteArray *data);
    //void updateStats(QJsonObject *data);
    void SendCommand(QByteArray command);
    void ExecCGIScriptViaGET(QString path, QUrlQuery query=QUrlQuery());
    void UploadDataViaPOST(QString path, QByteArray *DataToSend);
private:
    QTcpSocket *_pDevSocket;
    QTimer *ThreadTimer;
    QVector <QByteArray> *_pPendingCommands;
    QByteArray *_pReceivedTCPData;
    bool _pIsBusy;
    uint _pState;
    uint _pAccepted;
    uint _pRejected;
    uint _pUpTime;
    Pool *_pPools;
    double *_pBoardTemperature1;
    double *_pBoardTemperature2;
    double _pTemperature;
    double *_pBoardHashRateIdeal;
    double *_pBoardHashRateReal;
    double _pHashRate;
    uint *_pBoardHardwareErrors;
    uint _pHardwareErrors;
    uint *_pBoardFrequency;
    uint *_pBoardACN;
private slots:
    void CommandLoop();
    void on_socketConnected();
    void on_socketDisconnected();
    void on_socketTimeout();
    void on_socketError(QAbstractSocket::SocketError error);
    void readTcpData();
    void onAuthenticationNeeded(QNetworkReply *reply, QAuthenticator *authenticator);
    void onNetManagerFinished(QNetworkReply *reply);
};

#endif // ASICDEVICE_H
