#include "asicdevice.h"
#include "logger.hpp"
#include "main.hpp"

unsigned int ASICDevice::ActiveThreadsNum=0;

Pool::Pool()
{
    priority=0;
    StratumActive=false;
}

ASICDevice::ASICDevice(QObject *parent) : QObject(parent)
{
    _pIsBusy=false;
    CurrentOCProfile="none";
    APIPort=
    WebPort=
    GroupID=
    _pState=
    _pUpTime=
    _pHashRate=
    _pTemperature=
    _pHardwareErrors=0;
    _pReceivedTCPData=new(QByteArray);
    _pPendingCommands=new(QVector <QByteArray>);
    _pPools=new Pool[DEVICE_POOLS_NUM];
    _pBoardTemperature1=new double[DEVICE_BOARDS_NUM];
    _pBoardTemperature2=new double[DEVICE_BOARDS_NUM];
    _pBoardHashRateIdeal=new double[DEVICE_BOARDS_NUM];
    _pBoardHashRateReal=new double[DEVICE_BOARDS_NUM];
    _pBoardHardwareErrors=new uint[DEVICE_BOARDS_NUM];
    _pBoardFrequency=new uint[DEVICE_BOARDS_NUM];
    _pBoardACN=new uint[DEVICE_BOARDS_NUM];
    _pDevSocket=new QTcpSocket(this);
    ThreadTimer=new QTimer(this);
    for(int board=0; board<DEVICE_BOARDS_NUM; board++)
    {
        _pBoardTemperature1[board]=
        _pBoardTemperature2[board]=
        _pBoardHashRateIdeal[board]=
        _pBoardHashRateReal[board]=
        _pBoardHardwareErrors[board]=
        _pBoardFrequency[board]=
        _pBoardACN[board]=0;
    }
    connect(ThreadTimer, SIGNAL(timeout()), this, SLOT(on_socketTimeout()));
    connect(_pDevSocket, SIGNAL(readyRead()), this, SLOT(readTcpData()));
    connect(_pDevSocket, SIGNAL(connected()), this, SLOT(on_socketConnected()));
    connect(_pDevSocket, SIGNAL(disconnected()), this, SLOT(on_socketDisconnected()));
    connect(_pDevSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(on_socketError(QAbstractSocket::SocketError)));
    connect(this, SIGNAL(HaveAPendingCommands()), this, SLOT(CommandLoop()));
}

ASICDevice::~ASICDevice()
{
    delete[] _pPools;
    delete[] _pBoardTemperature1;
    delete[] _pBoardTemperature2;
    delete[] _pBoardHashRateIdeal;
    delete[] _pBoardHashRateReal;
    delete[] _pBoardHardwareErrors;
    delete[] _pBoardFrequency;
    delete[] _pBoardACN;
    delete(_pReceivedTCPData);
    delete(_pPendingCommands);
    delete(_pDevSocket);
    delete(ThreadTimer);
}

void ASICDevice::ExecCGIScriptViaGET(QString path, QUrlQuery query)
{
    QUrl ScriptURL;
    ScriptURL.setScheme("http");
    ScriptURL.setHost(this->Address.toString());
    ScriptURL.setPort(this->WebPort);
    ScriptURL.setUserName(this->UserName);
    ScriptURL.setPassword(this->Password);
    ScriptURL.setPath(path);
    if(!query.isEmpty())
    {
        ScriptURL.setQuery(query);
    }
    //gAppLogger->Log("sending GET request to");
    //gAppLogger->Log(ScriptURL.toString());
    QNetworkRequest ExecRequest;
    ExecRequest.setUrl(ScriptURL);
    ExecRequest.setHeader(QNetworkRequest::UserAgentHeader, API_HEADER_USER_AGENT);
    QNetworkAccessManager *netManager=new QNetworkAccessManager;
    connect(netManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(onNetManagerFinished(QNetworkReply *)));
    connect(netManager, SIGNAL(authenticationRequired(QNetworkReply *, QAuthenticator *)), this, SLOT(onAuthenticationNeeded(QNetworkReply *, QAuthenticator *)));
    netManager->get(ExecRequest);
}

void ASICDevice::UploadDataViaPOST(QString path, QByteArray *DataToSend)
{
    QUrl deviceURL;
    deviceURL.setScheme("http");
    deviceURL.setHost(this->Address.toString());
    deviceURL.setPort(this->WebPort);
    deviceURL.setUserName(this->UserName);
    deviceURL.setPassword(this->Password);
    deviceURL.setPath(path);
    QNetworkRequest setingsRequest;
    setingsRequest.setUrl(deviceURL);
    setingsRequest.setHeader(QNetworkRequest::UserAgentHeader, API_HEADER_USER_AGENT);
    setingsRequest.setHeader(QNetworkRequest::ContentTypeHeader, API_HEADER_CONTENT_TYPE);
    QNetworkAccessManager *netManager=new(QNetworkAccessManager);
    connect(netManager, SIGNAL(finished(QNetworkReply *)), this, SLOT(onNetManagerFinished(QNetworkReply *)));
    connect(netManager, SIGNAL(authenticationRequired(QNetworkReply *, QAuthenticator *)), this, SLOT(onAuthenticationNeeded(QNetworkReply *, QAuthenticator *)));
    //gAppLogger->Log("sending POST request to");
    //gAppLogger->Log(deviceURL.toString());
    netManager->post(setingsRequest, *DataToSend);
}

void ASICDevice::onNetManagerFinished(QNetworkReply *reply)
{
    QByteArray ReceivedData;
    if(reply->error()==QNetworkReply::NoError)
    {
        gAppLogger->Log("ASICDevice::onNetManagerFinished reply success");
    }
    else
    {
        gAppLogger->Log("ASICDevice::onNetManagerFinished reply error");
        gAppLogger->Log(reply->errorString());
    }
    QVariant statusCode=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if(statusCode.isValid())
    {
        gAppLogger->Log(Address.toString()+QString(" HttpStatusCode=")+QString::number(statusCode.toUInt()));
    }
    if(reply->isReadable())
    {
        ReceivedData=reply->readAll();
    }
    reply->manager()->disconnect();
    reply->manager()->deleteLater();
}

void ASICDevice::onAuthenticationNeeded(QNetworkReply *reply, QAuthenticator *authenticator)
{
    if(reply->error()==QNetworkReply::NoError)
    {
        //gAppLogger->Log("ASICDevice::onAuthenticationNeeded reply success");
    }
    else
    {
        //gAppLogger->Log("ASICDevice::onAuthenticationNeeded reply error");
        //gAppLogger->Log(reply->errorString());
    }
    authenticator->setPassword(Password);
    authenticator->setUser(UserName);
}

void ASICDevice::SendCommand(QByteArray command)
{
    gAppLogger->Log(QString("ASICDevice::SendCommand ")+
                   command);
    if(command.isEmpty())
    {
        return;
    }
    _pPendingCommands->append(command);
    emit(HaveAPendingCommands());
}

void ASICDevice::CommandLoop()
{
    if(_pIsBusy)
    {
        return;
    }
    _pIsBusy=true;
    ASICDevice::ActiveThreadsNum++;
    //gAppLogger->Log(QString("ASICDevice::ActiveThreadsNum ")+QString::number(ActiveThreadsNum));
    if(_pDevSocket->state()==QAbstractSocket::UnconnectedState)
    {
        ThreadTimer->setInterval(gAppConfig->ThreadLifeTime);
        ThreadTimer->start();
        _pDevSocket->connectToHost(Address, APIPort, QIODevice::ReadWrite);
    }
}

void ASICDevice::on_socketConnected()
{
    gAppLogger->Log(QString("ASICDevice::on_socketConnected ")+Address.toString());
    emit(SocketConnected(this));
    if(_pPendingCommands->count())
    {
        QByteArray cmd=_pPendingCommands->takeFirst();
        _pDevSocket->write(cmd);
    }
}

void ASICDevice::on_socketDisconnected()
{
    //gAppLogger->Log(QString("ASICDevice::on_socketDisconnected ")+Address.toString());
    ThreadTimer->stop();
    if(_pIsBusy)
    {
        _pIsBusy=false;
        ASICDevice::ActiveThreadsNum--;
        //gAppLogger->Log(QString("ASICDevice::ActiveThreadsNum ")+QString::number(ActiveThreadsNum));
    }
    if(_pPendingCommands->count())
    {
        emit(HaveAPendingCommands());
    }
}

void ASICDevice::on_socketTimeout()
{
    gAppLogger->Log(QString("ASICDevice::on_socketTimeout ")+Address.toString());
    ThreadTimer->stop();
    if(_pIsBusy)
    {
        _pIsBusy=false;
        ASICDevice::ActiveThreadsNum--;
        //gAppLogger->Log(QString("ASICDevice::ActiveThreadsNum ")+QString::number(ActiveThreadsNum));
    }
    if(_pDevSocket->state())
    {
        _pDevSocket->abort();
    }
    _pReceivedTCPData->clear();
    _pPendingCommands->clear();
    _pState|=ASICDevice::AlarmSocketTimeout;
    emit(Alarm(_pState));
}

void ASICDevice::on_socketError(QAbstractSocket::SocketError error)
{
    gAppLogger->Log(QString("ASICDevice::on_socketError ")+Address.toString());
    gAppLogger->Log(_pDevSocket->errorString()+
                   QString(" (SocketError code ")+
                   QString::number(error)+
                   QString(")"));
    if(_pIsBusy)
    {
        _pIsBusy=false;
        ASICDevice::ActiveThreadsNum--;
        //gAppLogger->Log(QString("ASICDevice::ActiveThreadsNum ")+QString::number(ActiveThreadsNum));
    }
    /*
    if(DevSocket->state())
    {
        DevSocket->abort();
    }
    _pReceivedTCPData->clear();
    _pPendingCommands->clear();
    */
    _pState|=ASICDevice::AlarmSocketError;
    emit(SocketError(this));
    emit(Alarm(_pState));
}

void ASICDevice::readTcpData()
{
    gAppLogger->Log(QString("ASICDevice::readTcpData ")+Address.toString());
    gAppLogger->Log(*_pReceivedTCPData);
    if(_pReceivedTCPData->at(_pReceivedTCPData->length()-1)==0 &&
       _pReceivedTCPData->at(_pReceivedTCPData->length()-2)==124)
    {
        updateStats(_pReceivedTCPData);
        _pReceivedTCPData->clear();
        _pDevSocket->close();
    }
}

void ASICDevice::updateStats(QByteArray *data)
{
    uint n, uval;
    double dval;
    char str[128], poolsubstr[512];
    for(int i=0; i<data->size(); i++)
    {
        if(2==sscanf(&data->data()[i], "freq_avg%u=%u", &n, &uval))
        {
            _pBoardFrequency[n-1]=uval;
            continue;
        }
        if(2==sscanf(&data->data()[i], "temp%u=%lf", &n, &dval))
        {
            _pBoardTemperature1[n-1]=dval;
            continue;
        }
        if(2==sscanf(&data->data()[i], "temp2_%u=%lf", &n, &dval))
        {
            _pBoardTemperature2[n-1]=dval;
            continue;
        }
        if(2==sscanf(&data->data()[i], "chain_rate%u=%lf", &n, &dval))
        {
            _pBoardHashRateReal[n-1]=dval;
            continue;
        }
        if(2==sscanf(&data->data()[i], "chain_rateideal%u=%lf", &n, &dval))
        {
            _pBoardHashRateIdeal[n-1]=dval;
            continue;
        }
        if(2==sscanf(&data->data()[i], "chain_acn%u=%u", &n, &uval))
        {
            _pBoardACN[n-1]=uval;
            continue;
        }
        if(2==sscanf(&data->data()[i], "chain_hw%u=%u", &n, &uval))
        {
            _pBoardHardwareErrors[n-1]=uval;
            continue;
        }
        if(1==sscanf(&data->data()[i], ",Elapsed=%u", &uval))
        {
            _pUpTime=uval;
            continue;
        }
        if(1==sscanf(&data->data()[i], ",Accepted=%u", &uval))
        {
            _pAccepted=uval;
            continue;
        }
        if(1==sscanf(&data->data()[i], ",Rejected=%u", &uval))
        {
            _pRejected=uval;
            continue;
        }
        if(1==sscanf(&data->data()[i], ",freq_avg=%u", &uval))
        {
            _pBoardFrequency[DEVICE_BOARDS_NUM-1]=uval;
            continue;
        }
        if(1==sscanf(&data->data()[i], ",Temperature=%lf", &dval))
        {
            _pTemperature=dval;
            continue;
        }
        if(1==sscanf(&data->data()[i], ",Hardware Errors=%u", &uval))
        {
            _pHardwareErrors=uval;
            continue;
        }
        if(1==sscanf(&data->data()[i], ",MHS 5s=%lf", &dval))
        {
            _pHashRate=dval;
            continue;
        }
        if(1==sscanf(&data->data()[i], ",GHS 5s=%lf", &dval))
        {
            _pHashRate=dval;
            continue;
        }
        if(1==sscanf(&data->data()[i], ",Type=%127[^,|]", str))
        {
            Type=str;
            continue;
        }
        if(1==sscanf(&data->data()[i], ",Miner=%127[^,|]", str))
        {
            Miner=str;
            continue;
        }
        if(1==sscanf(&data->data()[i], "CurrentOCProfile=%127[^|]", str))
        {
            CurrentOCProfile=str;
            continue;
        }
        if(2==sscanf(&data->data()[i], "|POOL=%u,%511[^|]", &uval, poolsubstr))
        {
            if(uval>=DEVICE_POOLS_NUM)
            {
                continue;
            }
            if(sscanf(poolsubstr, "URL=%127[^,]", str))
            {
                _pPools[uval].URL=QUrl(str);
            }
            for(uint i=0; i<strlen(poolsubstr) && !_pPools[uval].URL.isEmpty(); i++)
            {
                if(sscanf(&poolsubstr[i], "Status=%127[^,]", str))
                {
                    _pPools[uval].Status=str;
                    continue;
                }
                if(sscanf(&poolsubstr[i], "Stratum Active=%127[^,]", str))
                {
                    if(!strcmp(str, "true"))
                    {
                        _pPools[uval].StratumActive=true;
                    }
                    else
                    {
                        _pPools[uval].StratumActive=false;
                    }
                    continue;
                }
                if(sscanf(&poolsubstr[i], "User=%127[^,]", str))
                {
                    _pPools[uval].User=str;
                    continue;
                }
            }
            continue;
        }
    }
    emit(Updated(this));
}

void ASICDevice::ClearUpErrorFlags()
{
    _pState=0;
}

bool ASICDevice::IsAlarmed()
{
    return(_pState);
}

uint ASICDevice::State()
{
    return(_pState);
}

Pool *ASICDevice::ActivePool()
{
    for(int i=0; i<DEVICE_POOLS_NUM; i++)
    {
        if(_pPools[i].Status==QString("Alive"))
        {
            return(&_pPools[i]);
        }
    }
    return(&_pPools[0]);
}

uint ASICDevice::ActiveChipsNumber()
{
    uint ActiveChips, i;
    for(ActiveChips=0, i=0; i<DEVICE_BOARDS_NUM; i++)
    {
        ActiveChips+=_pBoardACN[i];
    }
    return(ActiveChips);
}

uint ASICDevice::Frequency()
{
    int i;
    uint Frequency, Div;
    for(Frequency=0, i=0, Div=0; i<DEVICE_BOARDS_NUM; i++)
    {
        if(_pBoardFrequency[i]>0)
        {
            Div++;
            Frequency+=_pBoardFrequency[i];
        }
    }
    if(Div>1)
    {
        Frequency/=Div;
    }
    return(Frequency);
}

uint ASICDevice::HardwareErrors()
{
    uint HardwareErrors, i;
    for(HardwareErrors=0, i=0; i<DEVICE_BOARDS_NUM; i++)
    {
        HardwareErrors+=_pBoardHardwareErrors[i];
    }
    if(_pHardwareErrors && HardwareErrors)
    {
        HardwareErrors+=_pHardwareErrors;
        HardwareErrors/=2;
    }
    return(HardwareErrors);
}

double ASICDevice::HashRate()
{
    double HashRate;
    int i;
    for(HashRate=0, i=0; i<DEVICE_BOARDS_NUM; i++)
    {
        HashRate+=_pBoardHashRateReal[i];
    }
    return(HashRate);
}

double ASICDevice::Temperature()
{
    int i;
    double Temperature, Div=0;
    for(Temperature=0, i=0; i<DEVICE_BOARDS_NUM; i++)
    {
        /*
        if(_pBoardTemperature1[i]>1.0 || _pBoardTemperature1[i]<-1.0)
        {
            Div++;
            Temperature+=_pBoardTemperature1[i];
        }
        */
        if(_pBoardTemperature2[i]>1.0 || _pBoardTemperature2[i]<-1.0)
        {
            Div++;
            Temperature+=_pBoardTemperature2[i];
        }
    }
    if(Div>0)
    {
        Temperature/=Div;
        return(Temperature);
    }
    return(_pTemperature);
}

uint ASICDevice::UpTime()
{
    return(_pUpTime);
}
