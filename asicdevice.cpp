#include "asicdevice.h"
#include "configurationholder.h"
#include "logger.hpp"
#include "main.hpp"

uint32_t ASICDevice::ActiveThreadsNum=0;

Pool::Pool()
{
    priority=0;
    StratumActive=false;
}

ASICDevice::ASICDevice(QObject *parent) : QObject(parent)
{
    _pIsBusy=false;
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
	connect(_pDevSocket, SIGNAL(readyRead()), this, SLOT(on_socketReadyRead()));
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
	gLogger->Log("ASICDevice::"+string(__FUNCTION__), LOG_DEBUG);
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
	//gLogger->Log("sending GET request to");
	//gLogger->Log(ScriptURL.toString());
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
	gLogger->Log("ASICDevice::"+string(__FUNCTION__), LOG_DEBUG);
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
	//gLogger->Log("sending POST request to");
	//gLogger->Log(deviceURL.toString());
    netManager->post(setingsRequest, *DataToSend);
}

void ASICDevice::onNetManagerFinished(QNetworkReply *reply)
{
	gLogger->Log("ASICDevice::"+string(__FUNCTION__), LOG_DEBUG);
    QByteArray ReceivedData;
    if(reply->error()==QNetworkReply::NoError)
    {
		gLogger->Log("ASICDevice::onNetManagerFinished reply success", LOG_INFO);
    }
    else
    {
		gLogger->Log("ASICDevice::onNetManagerFinished reply error", LOG_ERR);
		gLogger->Log(reply->errorString().toStdString(), LOG_ERR);
    }
    QVariant statusCode=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if(statusCode.isValid())
    {
		gLogger->Log(Address.toString().toStdString()+" HttpStatusCode="+to_string(statusCode.toInt()), LOG_INFO);
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
	gLogger->Log("ASICDevice::"+string(__FUNCTION__), LOG_DEBUG);
    if(reply->error()==QNetworkReply::NoError)
    {
		//gLogger->Log("ASICDevice::onAuthenticationNeeded reply success", LOG_INFO);
    }
    else
    {
//		gLogger->Log("ASICDevice::onAuthenticationNeeded reply error", LOG_ERR);
		//gLogger->Log(reply->errorString(), LOG_ERR);
    }
    authenticator->setPassword(Password);
    authenticator->setUser(UserName);
}

void ASICDevice::SendCommand(QByteArray command)
{
	gLogger->Log("ASICDevice::"+string(__FUNCTION__), LOG_DEBUG);
	gLogger->Log(Address.toString().toStdString()+" "+command.toStdString(), LOG_DEBUG);
    if(command.isEmpty())
    {
		gLogger->Log("An empty command has been put into the command queue", LOG_NOTICE);
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
    if(_pDevSocket->state()==QAbstractSocket::UnconnectedState)
    {
        ThreadTimer->setInterval(gAppConfig->ThreadLifeTime);
        ThreadTimer->start();
        _pDevSocket->connectToHost(Address, APIPort, QIODevice::ReadWrite);
    }
}

void ASICDevice::on_socketConnected()
{
	gLogger->Log("ASICDevice::"+string(__FUNCTION__), LOG_DEBUG);
	gLogger->Log(Address.toString().toStdString(), LOG_DEBUG);
    emit(SocketConnected(this));
    if(_pPendingCommands->count())
    {
        QByteArray cmd=_pPendingCommands->takeFirst();
        _pDevSocket->write(cmd);
    }
}

void ASICDevice::on_socketDisconnected()
{
	gLogger->Log("ASICDevice::"+string(__FUNCTION__), LOG_DEBUG);
	gLogger->Log(Address.toString().toStdString(), LOG_DEBUG);
    ThreadTimer->stop();
    if(_pIsBusy)
    {
        _pIsBusy=false;
        ASICDevice::ActiveThreadsNum--;
    }
    if(_pPendingCommands->count())
    {
        emit(HaveAPendingCommands());
    }
}

void ASICDevice::on_socketTimeout()
{
	gLogger->Log("ASICDevice::"+string(__FUNCTION__), LOG_DEBUG);
	gLogger->Log(Address.toString().toStdString(), LOG_DEBUG);
    ThreadTimer->stop();
    if(_pIsBusy)
    {
        _pIsBusy=false;
        ASICDevice::ActiveThreadsNum--;
//		gLogger->Log("ASICDevice::ActiveThreadsNum "+to_string(ActiveThreadsNum), LOG_DEBUG);
    }
    if(_pDevSocket->state())
    {
        _pDevSocket->abort();
    }
    _pReceivedTCPData->clear();
    _pPendingCommands->clear();
	_pState|=ASICAlarmFlags::AlarmSocketTimeout;
    emit(Alarm(_pState));
}

void ASICDevice::on_socketError(QAbstractSocket::SocketError error)
{
	gLogger->Log("ASICDevice::"+string(__FUNCTION__), LOG_DEBUG);
	gLogger->Log(Address.toString().toStdString(), LOG_DEBUG);
	gLogger->Log(_pDevSocket->errorString().toStdString()+
				(" (SocketError code ")+
				 to_string(error)+
				 (")"), LOG_DEBUG);
    if(_pIsBusy)
    {
        _pIsBusy=false;
        ASICDevice::ActiveThreadsNum--;
//		gLogger->Log("ASICDevice::ActiveThreadsNum "+to_string(ActiveThreadsNum), LOG_DEBUG);
    }
    /*
    if(DevSocket->state())
    {
        DevSocket->abort();
    }
    _pReceivedTCPData->clear();
    _pPendingCommands->clear();
    */
	_pState|=ASICAlarmFlags::AlarmSocketError;
    emit(SocketError(this));
    emit(Alarm(_pState));
}

void ASICDevice::on_socketReadyRead()
{
	gLogger->Log("ASICDevice::"+string(__FUNCTION__), LOG_DEBUG);
	gLogger->Log(Address.toString().toStdString(), LOG_DEBUG);
	*_pReceivedTCPData=_pDevSocket->readAll();
	gLogger->Log(_pReceivedTCPData->toStdString(), LOG_DEBUG);
	if(_pReceivedTCPData->length()<7)
	{
		gLogger->Log("Not enought data has been read from the socket ("+to_string(_pReceivedTCPData->length())+" bytes", LOG_ERR);
	}
	else
	{
		if(_pReceivedTCPData->at(_pReceivedTCPData->length()-1)==0 &&
		   _pReceivedTCPData->at(_pReceivedTCPData->length()-2)==124)
		{
			updateStats(_pReceivedTCPData);
		}
	}
	_pReceivedTCPData->clear();
	_pDevSocket->close();
}

void ASICDevice::updateStats(QByteArray *data)
{
	gLogger->Log("ASICDevice::"+string(__FUNCTION__), LOG_DEBUG);
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
		if(1==sscanf(&data->data()[i], ",Description=%127[^,|]", str))
		{
			Description=str;
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
