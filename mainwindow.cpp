#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cstring>
#include <math.h>

//The main window of a srh1224
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow){
    pCorrelatorClient = new QTcpSocket(this);
    QObject::connect(pCorrelatorClient, SIGNAL(connected()), this, SLOT(on_correlatorClient_connected()));
    QObject::connect(pCorrelatorClient, SIGNAL(readyRead()), this, SLOT(on_correlatorClient_parse()));
    QObject::connect(pCorrelatorClient, SIGNAL(disconnected()), this, SLOT(on_correlatorClient_disconnected()));

    pSyncDriverClient = new QTcpSocket(this);
    QObject::connect(pSyncDriverClient, SIGNAL(connected()), this, SLOT(on_SyncDriverClient_connected()));
    QObject::connect(pSyncDriverClient, SIGNAL(readyRead()), this, SLOT(on_SyncDriverClient_read()));
    QObject::connect(pSyncDriverClient, SIGNAL(disconnected()), this, SLOT(on_SyncDriverClient_disconnected()));
    packetNumber = 0;
    packetSize = 0;
    dataPacket = nullptr;

    QString soldatPath = QString("/home/svlesovoi/SSRT/soldat.txt");
//    QString soldatPath = QString("/home/sergey_lesovoi/SSRT/soldat.txt");
    soldat.fromFile(soldatPath);
    QDate now = QDate::currentDate();
    soldat.setDate(now);

    QSettings settings;

    correlatorIP = settings.value("network/correlatorIP", "10.0.5.179").toString();
    localOscillatorIP = settings.value("network/localOscillatorIP", "10.1.1.46").toString();
    correlatorPort = settings.value("network/correlatorPort", 56565).toInt();
    SyncDriverIP = settings.value("network/SyncDriverIP", "10.1.10.4").toString();
    SyncDriverPort = settings.value("network/SyncDriverPort", 56566).toInt();
    fitsPath = settings.value("FITS/fitsPath", "/home/sergey/SRH/currentData/").toString();
    fullPacketsInFits = settings.value("FITS/fullPacketsInFits", 256).toUInt();
    frequencyDelay = settings.value("array/frequencyDelay", 64).toUInt();
    dataDelay = settings.value("receiver/dataDelay", 10000).toUInt();
    dataDuration = settings.value("receiver/dataDuration", 2000000).toUInt();
    internalSync = settings.value("receiver/internalSync", 0).toBool();
    oneBitCorrelation = settings.value("receiver/oneBitCorrelation", 0).toBool();
    delayTracking = settings.value("receiver/delayTracking", 1).toBool();
    fringeStopping = settings.value("receiver/fringeStopping", 1).toBool();
    autoStart = settings.value("receiver/autoStart", 1).toBool();

    frequencyListSize = settings.value("FITS/frequencyListSize", 32).toUInt();
    frequencyList = new unsigned int[frequencyListSize];
    settings.beginReadArray("loFrequencies");
    for (unsigned int i = 0;i < frequencyListSize;++i){
        settings.setArrayIndex(i);
        frequencyList[i] = settings.value("loFrequency").toUInt();
    }
    settings.endArray();

    SyncDriverConfig.FSetDuration = settings.value("SyncDriver/frequencyPulseDuration", 10).toUInt();
    SyncDriverConfig.PolarToggle = settings.value("SyncDriver/frequencySwitchTime", 20).toUInt();
    SyncDriverConfig.PolarSet = settings.value("SyncDriver/frequencySetTime", 1000000).toUInt();
    SyncDriverConfig.HiTimeDuration = settings.value("SyncDriver/leftPolarizationDuration", 10000000).toUInt();
    SyncDriverConfig.LowTimeDuration = settings.value("SyncDriver/rightPolarizationDuration", 10000000).toUInt();
    SyncDriverConfig.NBand = settings.value("SyncDriver/frequencyBandsMask", 7).toUInt();
    SyncDriverConfig.NCycle = settings.value("SyncDriver/polarizationCyclesPerFrequency", 1).toUInt() - 1;
    SyncDriverConfig.NFSet = frequencyListSize - 1;

//--------------------------------------------------SouthAntennaDescriptor-------------------------------------------------------------------
    southAntennaNumber = settings.value("array/southAntennaNumber", 64).toUInt();
    pSouthAntennaName = new QString[southAntennaNumber];
    pSouthAntennaReceiver = new unsigned int[southAntennaNumber];
    pSouthAntennaChannel = new unsigned int[southAntennaNumber];
    pSouthAntennaFrontEndID = new unsigned int[southAntennaNumber];
    pSouthAntennaFeedID = new unsigned int[southAntennaNumber];
    pSouthAntennaDiameter = new float[southAntennaNumber];
    pSouthAntennaDelay = new unsigned int[southAntennaNumber];
    pSouthAntennaX = new int[southAntennaNumber];
    pSouthAntennaY = new int[southAntennaNumber];
    pSouthAntennaZ = new int[southAntennaNumber];
    pSouthAntennaIndex = new unsigned int[southAntennaNumber];

    settings.beginReadArray("SouthAntennaDescriptor");
    for (unsigned int i = 0;i < southAntennaNumber;++i){
        settings.setArrayIndex(i);
        QStringList n_r_c_x_y_z_fE_feed_D_T = settings.value("SouthAntennaDescriptor","S1001, 0, 0, 0, 0, 0, 0, 0, 0, 0").toString().split(",");
        pSouthAntennaName[i] = QString(n_r_c_x_y_z_fE_feed_D_T[0]);
        pSouthAntennaReceiver[i] = n_r_c_x_y_z_fE_feed_D_T[1].toUInt() - 1;
        pSouthAntennaChannel[i] = n_r_c_x_y_z_fE_feed_D_T[2].toUInt() - 1;
        pSouthAntennaX[i] = n_r_c_x_y_z_fE_feed_D_T[3].toInt();
        pSouthAntennaY[i] = n_r_c_x_y_z_fE_feed_D_T[4].toInt();
        pSouthAntennaZ[i] = n_r_c_x_y_z_fE_feed_D_T[5].toInt();
        pSouthAntennaFrontEndID[i] = n_r_c_x_y_z_fE_feed_D_T[6].toUInt();
        pSouthAntennaFeedID[i] = n_r_c_x_y_z_fE_feed_D_T[7].toUInt();
        pSouthAntennaDiameter[i] = n_r_c_x_y_z_fE_feed_D_T[8].toUInt();
        pSouthAntennaDelay[i] = n_r_c_x_y_z_fE_feed_D_T[9].toUInt();
        pSouthAntennaIndex[i] = pSouthAntennaReceiver[i]*16 + pSouthAntennaChannel[i];
    }
    settings.endArray();

//--------------------------------------------------EastAntennaDescriptor-------------------------------------------------------------------
    eastAntennaNumber = settings.value("array/eastAntennaNumber", 64).toUInt();
    pEastAntennaName = new QString[eastAntennaNumber];
    pEastAntennaReceiver = new unsigned int[eastAntennaNumber];
    pEastAntennaChannel = new unsigned int[eastAntennaNumber];
    pEastAntennaFrontEndID = new unsigned int[eastAntennaNumber];
    pEastAntennaFeedID = new unsigned int[eastAntennaNumber];
    pEastAntennaDiameter = new float[eastAntennaNumber];
    pEastAntennaDelay = new unsigned int[eastAntennaNumber];
    pEastAntennaX = new int[eastAntennaNumber];
    pEastAntennaY = new int[eastAntennaNumber];
    pEastAntennaZ = new int[eastAntennaNumber];
    pEastAntennaIndex = new unsigned int[eastAntennaNumber];

    settings.beginReadArray("EastAntennaDescriptor");
    for (unsigned int i = 0;i < eastAntennaNumber;++i){
        settings.setArrayIndex(i);
        QStringList n_r_c_x_y_z_fE_feed_D_T = settings.value("EastAntennaDescriptor","E1001, 0, 0, 0, 0, 0, 0, 0, 0, 0").toString().split(",");
        pEastAntennaName[i] = QString(n_r_c_x_y_z_fE_feed_D_T[0]);
        pEastAntennaReceiver[i] = n_r_c_x_y_z_fE_feed_D_T[1].toUInt() - 1;
        pEastAntennaChannel[i] = n_r_c_x_y_z_fE_feed_D_T[2].toUInt() - 1;
        pEastAntennaX[i] = n_r_c_x_y_z_fE_feed_D_T[3].toInt();
        pEastAntennaY[i] = n_r_c_x_y_z_fE_feed_D_T[4].toInt();
        pEastAntennaZ[i] = n_r_c_x_y_z_fE_feed_D_T[5].toInt();
        pEastAntennaFrontEndID[i] = n_r_c_x_y_z_fE_feed_D_T[6].toUInt();
        pEastAntennaFeedID[i] = n_r_c_x_y_z_fE_feed_D_T[7].toUInt();
        pEastAntennaDiameter[i] = n_r_c_x_y_z_fE_feed_D_T[8].toUInt();
        pEastAntennaDelay[i] = n_r_c_x_y_z_fE_feed_D_T[9].toUInt();
        pEastAntennaIndex[i] = pEastAntennaReceiver[i]*16 + pEastAntennaChannel[i];
    }
    settings.endArray();

//--------------------------------------------------WestAntennaDescriptor-------------------------------------------------------------------
    westAntennaNumber = settings.value("array/westAntennaNumber", 64).toUInt();
    pWestAntennaName = new QString[westAntennaNumber];
    pWestAntennaReceiver = new unsigned int[westAntennaNumber];
    pWestAntennaChannel = new unsigned int[westAntennaNumber];
    pWestAntennaFrontEndID = new unsigned int[westAntennaNumber];
    pWestAntennaFeedID = new unsigned int[westAntennaNumber];
    pWestAntennaDiameter = new float[westAntennaNumber];
    pWestAntennaDelay = new unsigned int[westAntennaNumber];
    pWestAntennaX = new int[westAntennaNumber];
    pWestAntennaY = new int[westAntennaNumber];
    pWestAntennaZ = new int[westAntennaNumber];
    pWestAntennaIndex = new unsigned int[westAntennaNumber];

    settings.beginReadArray("WestAntennaDescriptor");
    for (unsigned int i = 0;i < westAntennaNumber;++i){
        settings.setArrayIndex(i);
        QStringList n_r_c_x_y_z_fE_feed_D_T = settings.value("WestAntennaDescriptor","W1001, 0, 0, 0, 0, 0, 0, 0, 0, 0").toString().split(",");
        pWestAntennaName[i] = QString(n_r_c_x_y_z_fE_feed_D_T[0]);
        pWestAntennaReceiver[i] = n_r_c_x_y_z_fE_feed_D_T[1].toUInt() - 1;
        pWestAntennaChannel[i] = n_r_c_x_y_z_fE_feed_D_T[2].toUInt() - 1;
        pWestAntennaX[i] = n_r_c_x_y_z_fE_feed_D_T[3].toInt();
        pWestAntennaY[i] = n_r_c_x_y_z_fE_feed_D_T[4].toInt();
        pWestAntennaZ[i] = n_r_c_x_y_z_fE_feed_D_T[5].toInt();
        pWestAntennaFrontEndID[i] = n_r_c_x_y_z_fE_feed_D_T[6].toUInt();
        pWestAntennaFeedID[i] = n_r_c_x_y_z_fE_feed_D_T[7].toUInt();
        pWestAntennaDiameter[i] = n_r_c_x_y_z_fE_feed_D_T[8].toUInt();
        pWestAntennaDelay[i] = n_r_c_x_y_z_fE_feed_D_T[9].toUInt();
        pWestAntennaIndex[i] = pWestAntennaReceiver[i]*16 + pWestAntennaChannel[i];
    }
    settings.endArray();
//--------------------------------------------------CenterAntennaDescriptor-------------------------------------------------------------------
    settings.beginReadArray("CenterAntennaDescriptor");
    settings.setArrayIndex(0);
    QStringList n_r_c_x_y_z_fE_feed_D_T = settings.value("CenterAntennaDescriptor","C1001, 0, 0, 0, 0, 0, 0, 0, 0, 0").toString().split(",");
    centerAntennaName = QString(n_r_c_x_y_z_fE_feed_D_T[0]);
    centerAntennaReceiver = n_r_c_x_y_z_fE_feed_D_T[1].toUInt() - 1;
    centerAntennaChannel = n_r_c_x_y_z_fE_feed_D_T[2].toUInt() - 1;
    centerAntennaX = n_r_c_x_y_z_fE_feed_D_T[3].toInt();
    centerAntennaY = n_r_c_x_y_z_fE_feed_D_T[4].toInt();
    centerAntennaZ = n_r_c_x_y_z_fE_feed_D_T[5].toInt();
    centerAntennaFrontEndID = n_r_c_x_y_z_fE_feed_D_T[6].toUInt();
    centerAntennaFeedID = n_r_c_x_y_z_fE_feed_D_T[7].toUInt();
    centerAntennaDiameter = n_r_c_x_y_z_fE_feed_D_T[8].toUInt();
    centerAntennaDelay = n_r_c_x_y_z_fE_feed_D_T[9].toUInt();
    centerAntennaIndex = centerAntennaReceiver*16 + centerAntennaChannel;
    settings.endArray();
//--------------------------------------------------EastWestAntennaDescriptor---------------------------------------------------------------
    eastWestAntennaNumber = westAntennaNumber + 1 + eastAntennaNumber;
    pEastWestAntennaReceiver = new unsigned int[eastWestAntennaNumber];
    pEastWestAntennaChannel = new unsigned int[eastWestAntennaNumber];
    pEastWestAntennaName = new QString[eastWestAntennaNumber];
    unsigned int i;
    for (i = 0;i < westAntennaNumber;++i){
        pEastWestAntennaReceiver[i] = pWestAntennaReceiver[i];
        pEastWestAntennaChannel[i] = pWestAntennaChannel[i];
        pEastWestAntennaName[i] = pWestAntennaName[i];
    }

    pEastWestAntennaReceiver[i] = centerAntennaReceiver;
    pEastWestAntennaChannel[i] = centerAntennaChannel;
    pEastWestAntennaName[i] = centerAntennaName;

    for (++i;i < eastWestAntennaNumber;++i){
        pEastWestAntennaReceiver[i] = pEastAntennaReceiver[i - westAntennaNumber - 1];
        pEastWestAntennaChannel[i] = pEastAntennaChannel[i - westAntennaNumber - 1];
        pEastWestAntennaName[i] = pEastAntennaName[i - westAntennaNumber - 1];
    }
//--------------------------------------------------visIndexCalculation---------------------------------------------------------------
    southEastWestVisNumber = southAntennaNumber * (eastAntennaNumber + 1 + westAntennaNumber);
    southVisNumber = southAntennaNumber * (southAntennaNumber - 1)/2;
    eastWestVisNumber = (eastAntennaNumber + 1 + westAntennaNumber) * (eastAntennaNumber + westAntennaNumber)/2;
    vis01224Number = southEastWestVisNumber + southVisNumber + eastWestVisNumber;

    pVisIndToCorrPacketInd = new unsigned int[vis01224Number];
    pVisIndToFitsInd = new unsigned int[vis01224Number];
    pVisibilitySign = new int[vis01224Number];
    for (unsigned sig = 0;sig < vis01224Number;++sig) pVisibilitySign[sig] = 1;
    pVis01224AntennaNameA = new QString[vis01224Number];
    pVis01224AntennaNameB = new QString[vis01224Number];

    for (unsigned int i = 0;i < southAntennaNumber;++i){
        unsigned int southAntennaRow = pSouthAntennaName[i].right(2).toUInt() - 1;
        unsigned int southAntennaIndex = southAntennaRow * (eastAntennaNumber + westAntennaNumber);
        unsigned int H = pSouthAntennaReceiver[i]*16 + pSouthAntennaChannel[i];
//southWest vis
        for (unsigned j = 0;j < westAntennaNumber;++j){
            unsigned int westAntennaIndex = pWestAntennaName[j].right(2).toUInt() - 1;
            unsigned int visInd = i * (eastAntennaNumber + westAntennaNumber + 1) + j;
            pVisIndToFitsInd[visInd] = southAntennaIndex + (westAntennaNumber - westAntennaIndex - 1);
            unsigned int V = pWestAntennaReceiver[j]*16 + pWestAntennaChannel[j];
            pVisIndToCorrPacketInd[visInd] = visibilityIndexAsHV(H, V);
            pVisibilitySign[visInd] = H > V ? 1 : -1;
            pVis01224AntennaNameA[visInd] = pSouthAntennaName[i];
            pVis01224AntennaNameB[visInd] = pWestAntennaName[j];
        }
//southCenter vis
        unsigned int visInd = i * (eastAntennaNumber + westAntennaNumber + 1) + westAntennaNumber;
        pVisIndToFitsInd[visInd] = southAntennaIndex + westAntennaNumber;
        unsigned int V = centerAntennaReceiver*16 + centerAntennaChannel;
        pVisIndToCorrPacketInd[visInd] = visibilityIndexAsHV(H, V);
        pVisibilitySign[visInd] = H > V ? 1 : -1;
        pVis01224AntennaNameA[visInd] = pSouthAntennaName[i];
        pVis01224AntennaNameB[visInd] = centerAntennaName;

//southEast vis
        for (unsigned j = 0;j < eastAntennaNumber;++j){
            unsigned int eastAntennaIndex = pEastAntennaName[j].right(2).toUInt() - 1;
            unsigned int visInd = i * (eastAntennaNumber + westAntennaNumber + 1) + j + westAntennaNumber + 1;
            pVisIndToFitsInd[visInd] = southAntennaIndex + eastAntennaIndex + westAntennaNumber;
            unsigned int V = pEastAntennaReceiver[j]*16 + pEastAntennaChannel[j];
            pVisIndToCorrPacketInd[visInd] = visibilityIndexAsHV(H, V);
            pVisibilitySign[visInd] = H > V ? 1 : -1;
            pVis01224AntennaNameA[visInd] = pSouthAntennaName[i];
            pVis01224AntennaNameB[visInd] = pEastAntennaName[j];
        }
    }
//southSouth vis
    for (unsigned int i = 1, visInd = southEastWestVisNumber;i < southAntennaNumber;++i){
        for (unsigned int j = 0;j < southAntennaNumber - i;++j){
            unsigned int H = pSouthAntennaReceiver[j]*16 + pSouthAntennaChannel[j];
            unsigned int V = pSouthAntennaReceiver[j+i]*16 + pSouthAntennaChannel[j+i];
            pVisIndToFitsInd[visInd] = visInd;
            pVisIndToCorrPacketInd[visInd] = visibilityIndexAsHV(H, V);
            pVisibilitySign[visInd] = H > V ? 1 : -1;
            pVis01224AntennaNameA[visInd] = pSouthAntennaName[j];
            pVis01224AntennaNameB[visInd++] = pSouthAntennaName[j+i];
        }
    }
//eastWestEastWest vis
    for (unsigned int i = 1, visInd = southEastWestVisNumber + southVisNumber;i < eastWestAntennaNumber;++i){
        for (unsigned int j = 0;j < eastWestAntennaNumber - i;++j){
            unsigned int H = pEastWestAntennaReceiver[j]*16 + pEastWestAntennaChannel[j];
            unsigned int V = pEastWestAntennaReceiver[j+i]*16 + pEastWestAntennaChannel[j+i];
            pVisIndToFitsInd[visInd] = visInd;
            pVisIndToCorrPacketInd[visInd] = visibilityIndexAsHV(H, V);
            if (j < westAntennaNumber && (j + i) < westAntennaNumber)
                pVisibilitySign[visInd] = -1;
            else
                pVisibilitySign[visInd] = 1;
            pVis01224AntennaNameA[visInd] = pEastWestAntennaName[j];
            pVis01224AntennaNameB[visInd++] = pEastWestAntennaName[j+i];
        }
    }
//--------------------------------------------------ampIndexCalculation---------------------------------------------------------------
    amp01224Number = southAntennaNumber + westAntennaNumber + 1 + eastAntennaNumber;
    pAmpIndToCorrPacketInd = new unsigned int[amp01224Number];
    pAmpIndToFitsInd = new unsigned int[amp01224Number];
    pAntennaDelay = new unsigned int[amp01224Number];
    pAntennaName = new QString[amp01224Number];
    pAntennaReceiver = new unsigned int[amp01224Number];
    pAntennaReceiverChannel = new unsigned int[amp01224Number];

    unsigned int ant;
    for (ant = 0;ant < westAntennaNumber;++ant){
        pAmpIndToFitsInd[ant] = ant;
        pAntennaReceiver[ant] = pWestAntennaReceiver[ant];
        pAntennaReceiverChannel[ant] = pWestAntennaChannel[ant];
        pAmpIndToCorrPacketInd[ant] = pWestAntennaReceiver[ant]*16 + pWestAntennaChannel[ant];
        pAntennaName[ant] = pWestAntennaName[ant];
        pAntennaDelay[ant] = pWestAntennaDelay[ant];
    }
    pAmpIndToFitsInd[ant] = ant;
    pAntennaReceiver[ant] = centerAntennaReceiver;
    pAntennaReceiverChannel[ant] = centerAntennaChannel;
    pAmpIndToCorrPacketInd[ant] = centerAntennaReceiver*16 + centerAntennaChannel;
    pAntennaName[ant] = centerAntennaName;
    pAntennaDelay[ant] = centerAntennaDelay;
    for (++ant;ant < westAntennaNumber + eastAntennaNumber + 1;++ant){
        pAmpIndToFitsInd[ant] = ant;
        pAntennaReceiver[ant] = pEastAntennaReceiver[ant - westAntennaNumber - 1];
        pAntennaReceiverChannel[ant] = pEastAntennaChannel[ant - westAntennaNumber - 1];
        pAmpIndToCorrPacketInd[ant] = pEastAntennaReceiver[ant - westAntennaNumber - 1]*16 + pEastAntennaChannel[ant - westAntennaNumber - 1];
        pAntennaName[ant] = pEastAntennaName[ant - westAntennaNumber - 1];
        pAntennaDelay[ant] = pEastAntennaDelay[ant - westAntennaNumber - 1];
    }
    for (;ant < southAntennaNumber + westAntennaNumber + 1 + eastAntennaNumber;++ant){
        pAmpIndToFitsInd[ant] = ant;
        pAntennaReceiver[ant] = pSouthAntennaReceiver[ant - eastAntennaNumber - westAntennaNumber - 1];
        pAntennaReceiverChannel[ant] = pSouthAntennaChannel[ant - eastAntennaNumber - westAntennaNumber - 1];
        pAmpIndToCorrPacketInd[ant] = pSouthAntennaReceiver[ant - eastAntennaNumber - westAntennaNumber - 1]*16 + pSouthAntennaChannel[ant - eastAntennaNumber - westAntennaNumber - 1];
        pAntennaName[ant] = pSouthAntennaName[ant - eastAntennaNumber - westAntennaNumber - 1];
        pAntennaDelay[ant] = pSouthAntennaDelay[ant - eastAntennaNumber - westAntennaNumber - 1];
    }
//--------------------------------------------------rawDataAndFITS---------------------------------------------------------------
    numberOfFitsVisibilities = vis01224Number;
    numberOfFitsAmplitudes = amp01224Number;
    numberOfShowVisibilities = 4;
    currentFrequency = 0;
    currentPolarization = 0;
    showFrequency = 0;
    showPolarization = 0;
    lcpFullPacketNumber = 0;
    rcpFullPacketNumber = 0;
    fitsNumber = 0;
    parsingState = 0;
    maxVisValue = oneBitCorrelation ? 1 : 49; //4-bit correlation (+7..-7)
    visScale = 1./(dataDuration * maxVisValue);
    ampScale = visScale / 512;
    showAntennaA = 0;
    showAntennaB = 1;

    dataPacket = new unsigned char[numberOfFitsVisibilities * fullPacketsInFits * 2 * 4];

    freqColumn.resize(frequencyListSize);
    std::valarray<double> timeArr(fullPacketsInFits / frequencyListSize);
    std::valarray<int64_t> ampArr(fullPacketsInFits / frequencyListSize * numberOfFitsAmplitudes);
    std::valarray<complex<float> > visArr(fullPacketsInFits / frequencyListSize * numberOfFitsVisibilities);
    timeColumn.assign(frequencyListSize, timeArr);
    lcpAmpColumn.assign(frequencyListSize, ampArr);
    rcpAmpColumn.assign(frequencyListSize, ampArr);
    lcpVisColumn.assign(frequencyListSize, visArr);
    rcpVisColumn.assign(frequencyListSize, visArr);

    antIndexColumn.resize(amp01224Number);
    antNameColumn.resize(amp01224Number);
    antNameIndexColumn.resize(amp01224Number);
    antFrontEndColumn.resize(amp01224Number);
    antFeedColumn.resize(amp01224Number);
    antDiameterColumn.resize(amp01224Number);
    antXColumn.resize(amp01224Number);
    antYColumn.resize(amp01224Number);
    antZColumn.resize(amp01224Number);

    for (unsigned int i = 0;i < amp01224Number;++i){
        if (i < westAntennaNumber){
            antFrontEndColumn[i] = pWestAntennaFrontEndID[i];
            antFeedColumn[i] = pWestAntennaFeedID[i];
            antDiameterColumn[i] = pWestAntennaDiameter[i];
            antXColumn[i] = pWestAntennaX[i];
            antYColumn[i] = pWestAntennaY[i];
            antZColumn[i] = pWestAntennaZ[i];
            antIndexColumn[i] = i;
        } else if (i == westAntennaNumber){
            antFrontEndColumn[i] = centerAntennaFrontEndID;
            antFeedColumn[i] = centerAntennaFeedID;
            antDiameterColumn[i] = centerAntennaDiameter;
            antXColumn[i] = centerAntennaX;
            antYColumn[i] = centerAntennaY;
            antZColumn[i] = centerAntennaZ;
            antIndexColumn[i] = i;
        } else if (i < eastAntennaNumber + 1 + westAntennaNumber){
            antFrontEndColumn[i] = pEastAntennaFrontEndID[i - westAntennaNumber - 1];
            antFeedColumn[i] = pEastAntennaFeedID[i - westAntennaNumber - 1];
            antDiameterColumn[i] = pEastAntennaDiameter[i - westAntennaNumber - 1];
            antXColumn[i] = pEastAntennaX[i - westAntennaNumber - 1];
            antYColumn[i] = pEastAntennaY[i - westAntennaNumber - 1];
            antZColumn[i] = pEastAntennaZ[i - westAntennaNumber - 1];
            antIndexColumn[i] = i;
        } else if (i < southAntennaNumber + westAntennaNumber + 1 + eastAntennaNumber){
            antFrontEndColumn[i] = pSouthAntennaFeedID[i - eastAntennaNumber - westAntennaNumber - 1];
            antFeedColumn[i] = pSouthAntennaFrontEndID[i - eastAntennaNumber - westAntennaNumber - 1];
            antDiameterColumn[i] = pSouthAntennaDiameter[i - eastAntennaNumber - westAntennaNumber - 1];
            antXColumn[i] = pSouthAntennaX[i - eastAntennaNumber - westAntennaNumber - 1];
            antYColumn[i] = pSouthAntennaY[i - eastAntennaNumber - westAntennaNumber - 1];
            antZColumn[i] = pSouthAntennaZ[i - eastAntennaNumber - westAntennaNumber - 1];
            antIndexColumn[i] = i;
        }
        antNameColumn[i] = pAntennaName[i].toStdString();
        antNameIndexColumn[i] = QString::number(antIndexColumn[i]).toStdString();
    }
/*
    QFile fileLogMessages("fileLogMessages.txt");
    fileLogMessages.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream msg(&fileLogMessages);
    msg << "pAntennaName " << pAntennaName[0] << "\n";
    fileLogMessages.close();
*/
    for (ant = 0;ant < amp01224Number;++ant)
        antName2indexMap[pAntennaName[ant]] = antIndexColumn[ant];

    antAColumn.resize(vis01224Number);
    antBColumn.resize(vis01224Number);
    for (unsigned int vis = 0;vis < vis01224Number;++vis){
        antAColumn[vis] = antName2indexMap[pVis01224AntennaNameA[vis]];
        antBColumn[vis] = antName2indexMap[pVis01224AntennaNameB[vis]];
    }
//--------------------------------------------------fileLog---------------------------------------------------------------
        QFile fileLogIndexes("fileLogIndexes.txt");
        fileLogIndexes.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&fileLogIndexes);
        out << "westAntennaNumber " << westAntennaNumber << " eastAntennaNumber " << eastAntennaNumber << " southAntennaNumber " << southAntennaNumber << "\n";

        out << "southEastWestVisNumber " << southEastWestVisNumber << " southVisNumber " << southVisNumber << " eastWestVisNumber " << eastWestVisNumber << "\n";
        for (unsigned int i = 0;i < amp01224Number;++i)
                out << i + 1 << "\t" << pAntennaName[i] << "\t" << pAntennaReceiver[i] << "\t" << pAntennaReceiverChannel[i] << "\t" << pAmpIndToCorrPacketInd[i] << "\n";

	out << "\n Culmination" << "\n";
	out << soldat.DCulmination() << "\n";
	out << "\n Declination" << "\n";
	out << soldat.DDeclination() << "\n";

        out << "\n XYZT" << "\n";
        for (unsigned int ant = 0;ant < amp01224Number;++ant)
            out << pAntennaName[ant] << "\t" << antXColumn[ant] << "\t" << antYColumn[ant] << "\t" << antZColumn[ant] << "\t" << pAntennaDelay[ant] << "\n";

        out << "\nvis" << "\n";
        for (unsigned int i = 0;i < vis01224Number;++i)
                out << i << " "
                    << pVis01224AntennaNameA[i] << "\t("
                    << antName2indexMap[pVis01224AntennaNameA[i]] << ")\t"
                    << pVis01224AntennaNameB[i] << "\t("
                    << antName2indexMap[pVis01224AntennaNameB[i]] << ")\t"
                    << pVisIndToFitsInd[i] << "\t"
                    << pVisIndToCorrPacketInd[i] << "\n";

        fileLogIndexes.close();

//--------------------------------------------------UI---------------------------------------------------------------
    ui->setupUi(this);
    for(int vis = 0;vis < numberOfShowVisibilities;++vis)
        ui->plotter->addGraph();
    ui->plotter->xAxis->setLabel("packet number");
    ui->plotter->yAxis->setLabel("visibility");
    ui->plotter->legend->setVisible(true);

    ui->yScaleScroll->setRange(1,100);
    ui->yOffsetScroll->setRange(-50,50);
    ui->yScaleScroll->setValue(100);
    ui->yOffsetScroll->setValue(0);
    plotterYScale = 1.;
    plotterYOffset = 0.;
    ui->plotter->yAxis->setRange(-plotterYScale + plotterYOffset, plotterYScale + plotterYOffset);

    ui->xScaleScroll->setRange(1,10000);
    ui->xOffsetScroll->setRange(0,20000);
    ui->xScaleScroll->setValue(5000);
    ui->xOffsetScroll->setValue(0);
    plotterXScale = 5000;
    plotterXOffset = 0;
    ui->plotter->xAxis->setRange(plotterXOffset, plotterXScale + plotterXOffset);

    ui->plotter->graph(0)->setPen(QPen(QColor(255,0,0),1));
    ui->plotter->graph(1)->setPen(QPen(QColor(0,255,0),1));
    ui->plotter->graph(2)->setPen(QPen(QColor(0,0,255),1));
    ui->plotter->graph(3)->setPen(QPen(QColor(0,255,255),1));

    ui->plotter->graph(0)->setName(pAntennaName[0]);
    ui->plotter->graph(1)->setName(pAntennaName[1]);
    ui->plotter->graph(2)->setName(pAntennaName[0] + "," + pAntennaName[1] + " re");
    ui->plotter->graph(3)->setName(pAntennaName[0] + "," + pAntennaName[1] + " im");

    initAntennaArrayLayoutPlotter();

    ui->antennaPlotter->xAxis->setRange(0, amp01224Number);
    QSharedPointer<QCPAxisTickerFixed> antennaTicker(new QCPAxisTickerFixed);
    antennaTicker->setTickStep(16);
    ui->antennaPlotter->xAxis->setTicker(antennaTicker);
    ui->antennaPlotter->yAxis->setRange(0, 10);
    ui->antennaPlotter->addGraph();
    ui->antennaPlotter->graph(0)->setPen(QPen(QColor(0,0,0),1));
    ui->antennaPlotter->graph(0)->setLineStyle(QCPGraph::lsNone);
    ui->antennaPlotter->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));
    ui->antennaPlotter->graph(0)->setName("antenna amplitude");

    ui->antennaPlotter->addGraph();
    ui->antennaPlotter->graph(1)->setPen(QPen(QColor(255,0,0),1));
    ui->antennaPlotter->graph(1)->setLineStyle(QCPGraph::lsNone);
    ui->antennaPlotter->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));
    ui->antennaPlotter->graph(1)->setName("antenna A marker");

    ui->antennaPlotter->addGraph();
    ui->antennaPlotter->graph(2)->setPen(QPen(QColor(0,255,0),1));
    ui->antennaPlotter->graph(2)->setLineStyle(QCPGraph::lsNone);
    ui->antennaPlotter->graph(2)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));
    ui->antennaPlotter->graph(2)->setName("antenna B marker");

    ui->antennaADelaySpin->setRange(-20000,20000);
    ui->antennaADelaySpin->setValue(0);
    ui->antennaBDelaySpin->setRange(-20000,20000);
    ui->antennaBDelaySpin->setValue(0);
    ui->antennaGainSpin->setRange(0,20);
    ui->antennaGainSpin->setValue(10);
    ui->quantizerSpinBox->setRange(0, 10000000);
    ui->currentFrequencySpinBox->setRange(0, frequencyListSize - 1);
    ui->currentAntennaASpinBox->setRange(0, amp01224Number - 1);
    ui->currentAntennaBSpinBox->setRange(0, amp01224Number - 1);
    ui->currentAntennaASpinBox->setValue(showAntennaA);
    ui->currentAntennaBSpinBox->setValue(showAntennaB);
    ui->delayTrackingButton->setChecked(delayTracking);
    ui->fringeStoppingButton->setChecked(fringeStopping);
//--------------------------------------------------UI---------------------------------------------------------------
//    localOscillator = new QG7M(localOscillatorIP);
    if (autoStart){
        pCorrelatorClient->connectToHost(correlatorIP, static_cast<unsigned short>(correlatorPort));
    }
}

MainWindow::~MainWindow(){
}

void MainWindow::on_connectButton_clicked(bool checked){
    if (checked)
        pCorrelatorClient->connectToHost(correlatorIP, static_cast<unsigned short>(correlatorPort));
    else{
        initCorrelator(false);
        pCorrelatorClient->disconnectFromHost();
    }
}

void MainWindow::on_correlatorClient_connected(){
    static tPkg requestPacket;
    ui->logText->append(QString("correlator connected"));
    setTypicalPacketPrefix(&requestPacket, eRqst_GetProperty);
    pCorrelatorClient->write(reinterpret_cast<const char*>(&requestPacket), sizeof(tPkg_Head));
    if (autoStart){
//        ui->localOscillatorStartStop->setChecked(true);
//        on_localOscillatorStartStop_clicked(true);
        ui->connectButton->setChecked(true);
        ui->initCorrelatorButton->setChecked(true);
        initCorrelator(true);

        pSyncDriverClient->connectToHost(SyncDriverIP, static_cast<unsigned short>(SyncDriverPort));
    }
}

void MainWindow::on_correlatorClient_disconnected(){
    ui->logText->append(QString("correlator disconnected"));
}

void MainWindow::on_SyncDriverConnect_clicked(bool checked){
    if (checked)
        pSyncDriverClient->connectToHost(SyncDriverIP, SyncDriverPort);
    else{
        syncDriverStartStop(false);
        pSyncDriverClient->disconnectFromHost();
    }
}

void MainWindow::on_SyncDriverGetConfigButton_clicked(){
    static tSyncDriverPkg SyncDriverPacket;
    SyncDriverPacket.H.Rqst = eSyncDriverRqst_Rdr_GetCnfg;
    SyncDriverPacket.H.isPacked = 0;
    SyncDriverPacket.H.HeadExtTp = 0x00;
    SyncDriverPacket.H.Magic = 0x05;
    SyncDriverPacket.H.DtSz = 0;
    pSyncDriverClient->write(reinterpret_cast<const char*>(&SyncDriverPacket), sizeof(tSyncDriverPkg_Head) + SyncDriverPacket.H.DtSz);
}

void MainWindow::startSyncDriver(){
    ui->SyncDriverConnect->setChecked(true);
    ui->SyncDriverSetConfigButton->setChecked(true);
    syncDriverStartStop(true);
}

void MainWindow::on_SyncDriverClient_connected(){
    ui->logText->append(QString("SyncDriver connected"));
    if (autoStart){
        QTimer::singleShot(10000,this,&MainWindow::startSyncDriver);
    }
}

void MainWindow::on_SyncDriverClient_disconnected(){
    ui->logText->append(QString("SyncDriver disconnected"));
}

void MainWindow::on_SyncDriverSetConfigButton_clicked(bool checked){
    syncDriverStartStop(checked);
}

void MainWindow::syncDriverStartStop(bool start){
    static tSyncDriverPkg SyncDriverPacket;
    SyncDriverPacket.H.Rqst = eSyncDriverRqst_Rdr_SetCnfg;
    SyncDriverPacket.H.isPacked = 0;
    SyncDriverPacket.H.HeadExtTp = 0x00;
    SyncDriverPacket.H.Magic = 0x05;
    SyncDriverPacket.H.DtSz = sizeof(SyncDriverConfig);
    std::memset(SyncDriverPacket.D.pU8,0,dHlgrphSS_PkgDtMaxSz);
    SyncDriverPacket.D.Cfg.FSetDuration = SyncDriverConfig.FSetDuration;
    SyncDriverPacket.D.Cfg.PolarToggle = SyncDriverConfig.PolarToggle;
    SyncDriverPacket.D.Cfg.PolarSet = SyncDriverConfig.PolarSet;
    SyncDriverPacket.D.Cfg.HiTimeDuration = SyncDriverConfig.HiTimeDuration;
    SyncDriverPacket.D.Cfg.LowTimeDuration = SyncDriverConfig.LowTimeDuration;
    SyncDriverPacket.D.Cfg.NBand = SyncDriverConfig.NBand;
    SyncDriverPacket.D.Cfg.NCycle = SyncDriverConfig.NCycle;
    SyncDriverPacket.D.Cfg.NFSet = SyncDriverConfig.NFSet;
    if (start)
        SyncDriverPacket.D.Cfg.Cntrl = 0x11;
    else
        SyncDriverPacket.D.Cfg.Cntrl = 0x00;

    ui->logText->append("FSetDuration " + QString::number(SyncDriverPacket.D.Cfg.FSetDuration));
    ui->logText->append("PolarToggle " + QString::number(SyncDriverPacket.D.Cfg.PolarToggle));
    ui->logText->append("PolarSet " + QString::number(SyncDriverPacket.D.Cfg.PolarSet));
    ui->logText->append("HiTimeDuration " + QString::number(SyncDriverPacket.D.Cfg.HiTimeDuration));
    ui->logText->append("LowTimeDuration " + QString::number(SyncDriverPacket.D.Cfg.LowTimeDuration));
    ui->logText->append("NBand " + QString::number(SyncDriverPacket.D.Cfg.NBand));
    ui->logText->append("NCycle " + QString::number(SyncDriverPacket.D.Cfg.NCycle));
    ui->logText->append("NFSet " + QString::number(SyncDriverPacket.D.Cfg.NFSet));
    ui->logText->append("Cntrl " + QString::number(SyncDriverPacket.D.Cfg.Cntrl,16));

    pSyncDriverClient->write(reinterpret_cast<const char*>(&SyncDriverPacket), sizeof(tSyncDriverPkg_Head) + SyncDriverPacket.H.DtSz);
}

void MainWindow::on_correlatorClient_parse(){
    unsigned int lcpAmpColumnOffset;
    unsigned int rcpAmpColumnOffset;
    unsigned int lcpVisColumnOffset;
    unsigned int rcpVisColumnOffset;
    unsigned int bytesInBuffer = 0;
    qint64 currentPacketTime = 0;
    tPkg_Head* pCorrHead =  reinterpret_cast<tPkg_Head*>(correlatorRawBuffer);
    tPkg_Dt* pCorrData = reinterpret_cast<tPkg_Dt*>(correlatorRawBuffer + sizeof(tPkg_Head));
    tConfigure* pConf = reinterpret_cast<tConfigure*>(correlatorRawBuffer + sizeof(tPkg_Head));
    if (firstPacketParsing){
        bytesInBuffer = pCorrelatorClient->bytesAvailable();
        if (bytesInBuffer = pCorrelatorClient->bytesAvailable() <= 200000)
            pCorrelatorClient->read(reinterpret_cast<char*>(pCorrHead), bytesInBuffer);           //read first buffer
        else {
            while (bytesInBuffer > 200000){
                pCorrelatorClient->read(reinterpret_cast<char*>(pCorrHead), 200000);
                bytesInBuffer -= 200000;
            }
            pCorrelatorClient->read(reinterpret_cast<char*>(pCorrHead), bytesInBuffer);
        }
        firstPacketParsing = 0;
    } else if (parsingState == 0){
        if (static_cast<unsigned long>(pCorrelatorClient->bytesAvailable()) >= sizeof(tPkg_Head)){
            pCorrelatorClient->read(reinterpret_cast<char*>(pCorrHead), sizeof(tPkg_Head));           //read header
            switch (pCorrHead->Rqst){
                case eRqst_GetState:
                    ui->logText->append("eRqst_GetState");
                break;
                case eRqst_SetStateProcessing:
                    ui->logText->append("eRqst_SetStateProcessing");
                break;
                case eRqst_SetStateIdle:
                    ui->logText->append("eRqst_SetStateIdle");
                break;
                case eRqst_SetEmulSin:
                    ui->logText->append("eRqst_SetEmulSin");
                break;
                case eRqst_Rdr_SetCnfg:
                    ui->logText->append("eRqst_Rdr_SetCnfg");
                break;
                case eRqst_Rdr_SetRgs32:
                    ui->logText->append("eRqst_Rdr_SetRgs32");
                break;
                case eRqst_SetTime:
                    ui->logText->append("eRqst_SetTime");
                break;
                case eRqst_GetPropertyFPGA:
                    parsingState = 1;//there is payload
                    ui->logText->append("eRqst_GetPropertyFPGA");
                break;
                case eRqst_GetProperty:
                    parsingState = 1;//there is payload
                    ui->logText->append("eRqst_GetProperty");
                break;
                case eRqst_Rdr_Strm_DtOut:
                    parsingState = 2;//visibility data
                break;
            }
        }
    } else if (parsingState == 1){
        if (pCorrelatorClient->bytesAvailable() >= pCorrHead->DtSz){
            pCorrelatorClient->read(reinterpret_cast<char*>(pCorrData), pCorrHead->DtSz);         //read data
            parsingState = 0;
            switch (pCorrHead->Rqst){
                case eRqst_GetProperty:
                    ui->logText->append("eRqst_GetProperty");
                    ui->logText->append(QString(reinterpret_cast<const char*>(pCorrData->pU8)));
                break;
                case eRqst_GetPropertyFPGA:
                    ui->logText->append("eRqst_GetPropertyFPGA");
                    ui->logText->append(QString(reinterpret_cast<const char*>(pCorrData->pU8)));
                break;
            }
        }
    } else if (parsingState == 2){//read visibility data
        if (pCorrelatorClient->bytesAvailable() >= pCorrHead->DtSz){
            pCorrelatorClient->read(reinterpret_cast<char*>(pCorrData), pCorrHead->DtSz);         //read data
            if(pCorrData->Blck.DtBlck.DtSz > 0 && pCorrData->Blck.DtBlck.DtSz <= dHlgrph_PkgBlckDtMaxSz) { //visibility data
                if (pCorrData->Blck.DtBlck.Offset == 0){
                    std::memcpy(dataPacket + pCorrData->Blck.DtBlck.Offset, pCorrData->Blck.pU8 + sizeof(tConfigure) + sizeof(tDtBlock), pCorrData->Blck.DtBlck.DtSz);
                    currentPolarization = pCorrData->Blck.Cfg.InfoSpiDrv.StsSpi.Plrztn;
                    if (currentPolarization == showPolarization && currentFrequency == showFrequency){
                        ui->logText->append("Tbox " + QString::number(pCorrData->Blck.Cfg.Temprtr));
                        ui->logText->append("Tfpga " + QString::number(pCorrData->Blck.Cfg.InfoSpiDrv.StsSpi.Tfpga - 128));
                    }
                    currentFrequency = 0;
                    for (unsigned int f = 0;f < frequencyListSize;++f)
                        if (pCorrData->Blck.Cfg.InfoSpiDrv.Frequency == frequencyList[f]){
                                currentFrequency = f;
                        }

                    currentPacketTime = pCorrData->Blck.Cfg.InfoSpiDrv.Time * 1000L + pCorrData->Blck.Cfg.InfoSpiDrv.TimePrescaler / 100000L;
//********
                    QTime curT = QTime::currentTime();
                    QDateTime corrT = QDateTime::fromMSecsSinceEpoch(currentPacketTime);
                    ui->logText->append("Local time " + QString::number(curT.msecsSinceStartOfDay()));
                    ui->logText->append("Packet time " + QString::number(corrT.time().msecsSinceStartOfDay()));
//********

                } else
                    std::memcpy(dataPacket + pCorrData->Blck.DtBlck.Offset, pCorrData->Blck.pU8, pCorrData->Blck.DtBlck.DtSz);

                if (pCorrData->Blck.DtBlck.Offset + pCorrData->Blck.DtBlck.DtSz == pCorrData->Blck.DtBlck.FullDtSz){
                    ++packetNumber;
                    if (packetNumber > plotterXScale)
                        ui->plotter->xAxis->setRange(packetNumber - plotterXScale + plotterXOffset, packetNumber + plotterXOffset);

                    int32_t* pAmpl = reinterpret_cast<int32_t*>(dataPacket + sizeof(tConfigure)) + 44096;
                    int64_t* pAnt0 = reinterpret_cast<int64_t*>(pAmpl);
                    if (currentPolarization == showPolarization && currentFrequency == showFrequency){
                        ui->currentFrequencySpinBox->setPrefix(QString::number(frequencyList[currentFrequency]) + " ");
                        ui->plotter->graph(0)->addData(packetNumber, pAnt0[pAmpIndToCorrPacketInd[showAntennaA]] * ampScale);
                        ui->plotter->graph(1)->addData(packetNumber, pAnt0[pAmpIndToCorrPacketInd[showAntennaB]] * ampScale);
                        unsigned int visibilityIndex = visibilityIndexAsHV(pAntennaReceiver[showAntennaA]*16 + pAntennaReceiverChannel[showAntennaA],
                                                                           pAntennaReceiver[showAntennaB]*16 + pAntennaReceiverChannel[showAntennaB]);
                        if (visibilityIndex < numberOfFitsVisibilities){
                            ui->plotter->graph(2)->addData(packetNumber, *(reinterpret_cast<int32_t*>(dataPacket  + sizeof(tConfigure)) + visibilityIndex*2)*visScale);
                            ui->plotter->graph(3)->addData(packetNumber, *(reinterpret_cast<int32_t*>(dataPacket  + sizeof(tConfigure)) + visibilityIndex*2 + 1)*visScale);
                        }
                        QVector<double> antennaX(numberOfFitsAmplitudes);
                        QVector<double> antennaY(numberOfFitsAmplitudes);
                        QVector<double> antennaAXmarker(1);
                        QVector<double> antennaAYmarker(1);
                        QVector<double> antennaBXmarker(1);
                        QVector<double> antennaBYmarker(1);
                        for(unsigned int ant = 0;ant < numberOfFitsAmplitudes;++ant){
                          antennaX[ant] = ant;
                          antennaY[ant] = static_cast<double>(pAnt0[pAmpIndToCorrPacketInd[ant]] * ampScale);
                          if (ant == showAntennaA){
                              antennaAXmarker[0] = antennaX[ant];
                              antennaAYmarker[0] = antennaY[ant];
                          }
                          if (ant == showAntennaB){
                              antennaBXmarker[0] = antennaX[ant];
                              antennaBYmarker[0] = antennaY[ant];
                          }
                        }
                        ui->antennaPlotter->graph(0)->setData(antennaX, antennaY, numberOfFitsAmplitudes);
                        ui->antennaPlotter->graph(1)->setData(antennaAXmarker, antennaAYmarker, 1);
                        ui->antennaPlotter->graph(2)->setData(antennaBXmarker, antennaBYmarker, 1);
                        ui->antennaPlotter->replot();
                    } else {
                    }
                    ui->plotter->replot();
                    lcpAmpColumnOffset = lcpFullPacketNumber / frequencyListSize * numberOfFitsAmplitudes;
                    rcpAmpColumnOffset = rcpFullPacketNumber / frequencyListSize * numberOfFitsAmplitudes;
                    lcpVisColumnOffset = lcpFullPacketNumber / frequencyListSize * numberOfFitsVisibilities;
                    rcpVisColumnOffset = rcpFullPacketNumber / frequencyListSize * numberOfFitsVisibilities;

                    QTime curTime = QTime::currentTime();
                    QDateTime packetDateTime = QDateTime::fromMSecsSinceEpoch(currentPacketTime);
                    int64_t* pAmplitude = pAnt0;
                    int64_t* pVisibility = reinterpret_cast<int64_t*>(dataPacket  + sizeof(tConfigure));
                    if (currentPolarization == 1 && lcpFullPacketNumber < fullPacketsInFits){
                        QString msg;
                        freqColumn[currentFrequency] = frequencyList[currentFrequency];
//                        timeColumn[currentFrequency][lcpFullPacketNumber / frequencyListSize] = curTime.msecsSinceStartOfDay() * 0.001;
                        timeColumn[currentFrequency][lcpFullPacketNumber / frequencyListSize] = packetDateTime.time().msecsSinceStartOfDay() * 0.001;
                        for(unsigned int lcpAmp = 0;lcpAmp < numberOfFitsAmplitudes;++lcpAmp)
                            lcpAmpColumn[currentFrequency][lcpAmpColumnOffset + pAmpIndToFitsInd[lcpAmp]] = pAmplitude[pAmpIndToCorrPacketInd[lcpAmp]];
                        for(unsigned int lcpVis = 0;lcpVis < numberOfFitsVisibilities;++lcpVis){
                            int64_t lcpInt64 = pVisibility[pVisIndToCorrPacketInd[lcpVis]];
                            float realLcpInt64 = *(int32_t*)&lcpInt64;
                            float imagLcpInt64 = *((int32_t*)&lcpInt64 + 1);
//                            lcpVisColumn[currentFrequency][lcpVisColumnOffset + pVisIndToFitsInd[lcpVis]] = complex<float>(realLcpInt64,  imagLcpInt64);
                            lcpVisColumn[currentFrequency][lcpVisColumnOffset + lcpVis] = complex<float>(realLcpInt64,  imagLcpInt64);
                        }
                        ++lcpFullPacketNumber;
                    } else if (rcpFullPacketNumber < fullPacketsInFits){
                        QString msg;
//                        msg.sprintf("RCP frequency %d", currentFrequency);
//                        ui->logText->append(msg);
                        for(unsigned int rcpAmp = 0;rcpAmp < numberOfFitsAmplitudes;++rcpAmp)
                            rcpAmpColumn[currentFrequency][rcpAmpColumnOffset + pAmpIndToFitsInd[rcpAmp]] = pAmplitude[pAmpIndToCorrPacketInd[rcpAmp]];
                        for(unsigned int rcpVis = 0;rcpVis < numberOfFitsVisibilities;++rcpVis){
                            int64_t rcpInt64 = pVisibility[pVisIndToCorrPacketInd[rcpVis]];
                            float realRcpInt64 = *(int32_t*)&rcpInt64;
                            float imagRcpInt64 = *((int32_t*)&rcpInt64 + 1);
//                            rcpVisColumn[currentFrequency][rcpVisColumnOffset + pVisIndToFitsInd[rcpVis]] = complex<float>(realRcpInt64, imagRcpInt64);
                            rcpVisColumn[currentFrequency][rcpVisColumnOffset + rcpVis] = complex<float>(realRcpInt64, imagRcpInt64);
                        }
                        ++rcpFullPacketNumber;
                    }
                    if (lcpFullPacketNumber == fullPacketsInFits && rcpFullPacketNumber == fullPacketsInFits){
                        writeCurrentFits();
                        ++fitsNumber;
                        currentFrequency = 0;
                        lcpFullPacketNumber = 0;
                        rcpFullPacketNumber = 0;
                    }
                }//full packet read
            }//visibility data
            parsingState = 0;
        }
    }
}

void MainWindow::addKey2FitsHeader(QString key, QString value, QString comment, FITS* pFits){
    pFits->pHDU().addKey(key.toStdString(),value.toStdString(),comment.toStdString());
}

void MainWindow::addKey2FitsHeader(QString key, int value, FITS* pFits){
    pFits->pHDU().addKey(key.toStdString(),value,"");
}

void MainWindow::writeCurrentFits(){
    int rows(frequencyListSize);
    string antPairHduSign("SRH_ANT_PAIRS");
    string antNameHduSign("SRH_ANT_NAMES");
    string antHduSign("SRH_ANT");
    string hduSign("SRH_DATA");
    QString qFitsName = "currentData/SRH1224/" + QDateTime::currentDateTime().toString("yyyyMMdd") + "/srh_1224_" + QDateTime::currentDateTime().toString("yyyyMMddThhmmss") + ".fit";
    const std::string fitsName = qFitsName.toStdString();
    std::vector<string> antNameColName(2,"");
    std::vector<string> antNameColForm(2,"");
    std::vector<string> antNameColUnit(2,"");

    std::vector<string> antColName(7,"");
    std::vector<string> antColForm(7,"");
    std::vector<string> antColUnit(7,"");

    std::vector<string> antPairColName(2,"");
    std::vector<string> antPairColForm(2,"");
    std::vector<string> antPairColUnit(2,"");

    std::vector<string> colName(6,"");
    std::vector<string> colForm(6,"");
    std::vector<string> colUnit(6,"");

    QString columnFormat;

    antNameColName[0] = "ant_name";
    antNameColName[1] = "ant_index";

    antNameColForm[0] = "A5";
    antNameColForm[1] = "A3";

    antNameColUnit[0] = "";
    antNameColUnit[1] = "";

//------
    antColName[0] = "ant_index";
    antColName[1] = "ant_X";
    antColName[2] = "ant_Y";
    antColName[3] = "ant_Z";
    antColName[4] = "ant_fe_id";
    antColName[5] = "ant_feed_id";
    antColName[6] = "ant_diameter";

    antColForm[0] = "1J";
    antColForm[1] = "1J";
    antColForm[2] = "1J";
    antColForm[3] = "1J";
    antColForm[4] = "1J";
    antColForm[5] = "1J";
    antColForm[6] = "1K";

    antColUnit[0] = "";
    antColUnit[1] = "mm";
    antColUnit[2] = "mm";
    antColUnit[3] = "mm";
    antColUnit[4] = "";
    antColUnit[5] = "";
    antColUnit[6] = "m";

//------
    antPairColName[0] = "ant_A";
    antPairColName[1] = "ant_B";

    antPairColForm[0] = "1J";
    antPairColForm[1] = "1J";

    antPairColUnit[0] = "";
    antPairColUnit[1] = "";

//------
    colName[0] = "frequency";
    colName[1] = "time";
    colName[2] = "amp_lcp";
    colName[3] = "amp_rcp";
    colName[4] = "vis_lcp";
    colName[5] = "vis_rcp";

    colForm[0] = "1J";
    columnFormat.sprintf("%dD", fullPacketsInFits / frequencyListSize);                             colForm[1] = columnFormat.toStdString();
    columnFormat.sprintf("%dK", fullPacketsInFits / frequencyListSize * numberOfFitsAmplitudes);  colForm[2] = columnFormat.toStdString();
    columnFormat.sprintf("%dK", fullPacketsInFits / frequencyListSize * numberOfFitsAmplitudes);  colForm[3] = columnFormat.toStdString();
    columnFormat.sprintf("%dC", fullPacketsInFits / frequencyListSize * numberOfFitsVisibilities);  colForm[4] = columnFormat.toStdString();
    columnFormat.sprintf("%dC", fullPacketsInFits / frequencyListSize * numberOfFitsVisibilities);  colForm[5] = columnFormat.toStdString();

    colUnit[0] = "Hz";
    colUnit[1] = "second";
    colUnit[2] = "watt";
    colUnit[3] = "watt";
    colUnit[4] = "correlation";
    colUnit[5] = "correlation";

    FITS fits2save(fitsName, Write);
    addKey2FitsHeader(QString("ORIGIN"),QString("ISTP"),QString("Institue of Solar-Terrestrial Physics"),&fits2save);
    addKey2FitsHeader(QString("TELESCOP"),QString("SSRT"),QString("Siberian Solar Radio Telescope"),&fits2save);
    addKey2FitsHeader(QString("INSTRUME"),QString("SRH01224"),QString("3-6 GHz Siberian Radioheliograph"),&fits2save);
    addKey2FitsHeader(QString("OBJECT"),QString("The Sun"),QString(""),&fits2save);
    addKey2FitsHeader(QString("OBS-LAT"),QString("51.759"),QString("Observatory latitude N51.759d"),&fits2save);
    addKey2FitsHeader(QString("OBS-LONG"),QString("102.217"),QString("Observatory longitude E102.217d"),&fits2save);
    addKey2FitsHeader(QString("OBS-ALT"),QString("799"),QString("Observatory altitude 799m asl"),&fits2save);
    addKey2FitsHeader(QString("FR_CHAN"),QString("10"),QString("Frequency channel width 10MHz"),&fits2save);
    addKey2FitsHeader(QString("VIS_MAX"),QString::number(dataDuration*maxVisValue),QString("Visibility scale V /= VIS_MAX"),&fits2save);

    QDate currentDate = QDate::currentDate();
    addKey2FitsHeader(QString("DATE-OBS"),currentDate.toString(Qt::ISODate),QString(""),&fits2save);
    QTime currentTime = QTime::currentTime();
    addKey2FitsHeader(QString("TIME-OBS"),currentTime.toString(QString("HH:mm:ss")),QString(""),&fits2save);

    try{
        Table* pTable = fits2save.addTable(hduSign, rows, colName, colForm, colUnit);
        pTable->column(colName[0]).write(freqColumn, 1);
        pTable->column(colName[1]).writeArrays(timeColumn, 1);
        pTable->column(colName[2]).writeArrays(lcpAmpColumn, 1);
        pTable->column(colName[3]).writeArrays(rcpAmpColumn, 1);
        pTable->column(colName[4]).writeArrays(lcpVisColumn, 1);
        pTable->column(colName[5]).writeArrays(rcpVisColumn, 1);
    } catch (FITS::CantCreate){
        ui->logText->append("FITS data table error");
    }

    try{
        Table* pAntNameTable = fits2save.addTable(antNameHduSign, amp01224Number, antNameColName, antNameColForm, antNameColUnit, AsciiTbl);
        pAntNameTable->column(antNameColName[0]).write(antNameColumn, 1);
        pAntNameTable->column(antNameColName[1]).write(antNameIndexColumn, 1);
    } catch (FITS::CantCreate){
        ui->logText->append("FITS antenna name table error");
    }

    try{
        Table* pAntTable = fits2save.addTable(antHduSign, amp01224Number, antColName, antColForm, antColUnit);
        pAntTable->column(antColName[0]).write(antIndexColumn, 1);
        pAntTable->column(antColName[1]).write(antXColumn, 1);
        pAntTable->column(antColName[2]).write(antYColumn, 1);
        pAntTable->column(antColName[3]).write(antZColumn, 1);
        pAntTable->column(antColName[4]).write(antFrontEndColumn, 1);
        pAntTable->column(antColName[5]).write(antFeedColumn, 1);
        pAntTable->column(antColName[6]).write(antDiameterColumn, 1);
        fitsNumber++;
    } catch (FITS::CantCreate){
        ui->logText->append("FITS antenna table error");
    }

    try{
        Table* pAntPairTable = fits2save.addTable(antPairHduSign, vis01224Number, antPairColName, antPairColForm, antPairColUnit);
        pAntPairTable->column(antPairColName[0]).write(antAColumn, 1);
        pAntPairTable->column(antPairColName[1]).write(antBColumn, 1);
    } catch (FITS::CantCreate){
        ui->logText->append("FITS antenna pair table error");
    }
}

void MainWindow::on_initReceiversButton_clicked(){
    initDigitalReceivers();
}

void MainWindow::initDigitalReceivers(){
    static tPkg setRgPacket;
    setTypicalPacketPrefix(&setRgPacket, eRqst_Rdr_SetRgs32);

    setRgPacket.D.Rg32.Count = 75;
    setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;
    setRgPacket.D.Rg32.Rg[0] = 0x0F; //receiver chip select address
    setRgPacket.D.Rg32.Rg[1] = 0x1FFF; //receiver selected
    setRgPacket.D.Rg32.Rg[2] = 0x08; //internal receiver address (0x08 FIFO)
    setRgPacket.D.Rg32.Rg[3] = 0x0B9;
    setRgPacket.D.Rg32.Rg[4] = static_cast<uint32_t>((soldat.DCulmination() - static_cast<int>(soldat.DCulmination())) * 1e8); //noon as ticks
    setRgPacket.D.Rg32.Rg[5] = static_cast<uint32_t>(soldat.DCulmination()); //noon as seconds
    *(reinterpret_cast<float*>(&setRgPacket.D.Rg32.Rg[6])) = static_cast<float>(soldat.DDHdt()); //dHdt
    *(reinterpret_cast<float*>(&setRgPacket.D.Rg32.Rg[7])) = static_cast<float>(soldat.DD2Hdt2()); //d2Hdt2
    *(reinterpret_cast<float*>(&setRgPacket.D.Rg32.Rg[8])) = static_cast<float>(soldat.DDeclination()); //declination
    *(reinterpret_cast<float*>(&setRgPacket.D.Rg32.Rg[9])) = static_cast<float>(soldat.DDDdt()); //dDdt
    *(reinterpret_cast<float*>(&setRgPacket.D.Rg32.Rg[10])) = static_cast<float>(soldat.DD2Ddt2());  //d2Ddt2
    for (int rec = 0;rec < 13;++rec){
        setRgPacket.D.Rg32.Rg[1] = 1 << pAntennaReceiver[rec*16]; //receiver selected
        ui->logText->append("DR " + QString::number(setRgPacket.D.Rg32.Rg[1]));
        for(int channel = 0;channel < 16;++channel){
            unsigned int r_c_ind = rec*16 + channel;
            setRgPacket.D.Rg32.Rg[channel*4 + 11] = antXColumn[r_c_ind]; //
            setRgPacket.D.Rg32.Rg[channel*4 + 12] = antYColumn[r_c_ind]; //
            setRgPacket.D.Rg32.Rg[channel*4 + 13] = antZColumn[r_c_ind]; //
            setRgPacket.D.Rg32.Rg[channel*4 + 14] = pAntennaDelay[r_c_ind]; //
        }
        ui->logText->append("sent " + QString::number(pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz)));
    }

    ui->logText->append("Culmination " + QString::number(soldat.DCulmination()));
    ui->logText->append("Declination " + QString::number(soldat.DDeclination()));
    setBitWindowPosition(10);
}

void MainWindow::initEphemeris(){
    static tPkg setRgPacket;
    setTypicalPacketPrefix(&setRgPacket, eRqst_Rdr_SetRgs32);

    setRgPacket.D.Rg32.Count = 11;
    setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;
    setRgPacket.D.Rg32.Rg[0] = 0x0F; //receiver chip select address
    setRgPacket.D.Rg32.Rg[1] = 0x1FFF; //receiver selected
    setRgPacket.D.Rg32.Rg[2] = 0x08; //internal receiver address (0x08 FIFO)
    setRgPacket.D.Rg32.Rg[3] = 0x0B9;
    setRgPacket.D.Rg32.Rg[4] = static_cast<uint32_t>((soldat.DCulmination() - static_cast<int>(soldat.DCulmination())) * 1e8); //noon as ticks
    setRgPacket.D.Rg32.Rg[5] = static_cast<uint32_t>(soldat.DCulmination()); //noon as seconds
    *(reinterpret_cast<float*>(&setRgPacket.D.Rg32.Rg[6])) = static_cast<float>(soldat.DDHdt()); //dHdt
    *(reinterpret_cast<float*>(&setRgPacket.D.Rg32.Rg[7])) = static_cast<float>(soldat.DD2Hdt2()); //d2Hdt2
    *(reinterpret_cast<float*>(&setRgPacket.D.Rg32.Rg[8])) = static_cast<float>(soldat.DDeclination()); //declination
    *(reinterpret_cast<float*>(&setRgPacket.D.Rg32.Rg[9])) = static_cast<float>(soldat.DDDdt()); //dDdt
    *(reinterpret_cast<float*>(&setRgPacket.D.Rg32.Rg[10])) = static_cast<float>(soldat.DD2Ddt2());  //d2Ddt2
    pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
}

void MainWindow::initCoordinates(){
    static tPkg setRgPacket;
    setTypicalPacketPrefix(&setRgPacket, eRqst_Rdr_SetRgs32);

    setRgPacket.D.Rg32.Count = 68;
    setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;
    setRgPacket.D.Rg32.Rg[0] = 0x0F; //receiver chip select address
    setRgPacket.D.Rg32.Rg[1] = 0x1FFF; //receiver selected
    setRgPacket.D.Rg32.Rg[2] = 0x08; //internal receiver address (0x08 FIFO)
    setRgPacket.D.Rg32.Rg[3] = 0x0C0;
    for (int rec = 0;rec < 13;++rec){
        setRgPacket.D.Rg32.Rg[1] = 1 << pAntennaReceiver[rec*16]; //receiver selected
        for(int channel = 0;channel < 16;++channel){
            unsigned int r_c_ind = rec*16 + channel;
            setRgPacket.D.Rg32.Rg[channel*4 + 4] = antXColumn[r_c_ind]; //
            setRgPacket.D.Rg32.Rg[channel*4 + 5] = antYColumn[r_c_ind]; //
            setRgPacket.D.Rg32.Rg[channel*4 + 6] = antZColumn[r_c_ind]; //
            setRgPacket.D.Rg32.Rg[channel*4 + 7] = 0;//pAntennaDelay[r_c_ind]; //
        }
        pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
    }
}

void MainWindow::setBitWindowPosition(unsigned int bitWindowPosition){
    static tPkg setRgPacket;
    setTypicalPacketPrefix(&setRgPacket, eRqst_Rdr_SetRgs32);
    setRgPacket.D.Rg32.Rg[0] = 0x0F; //receiver chip select address
    setRgPacket.D.Rg32.Rg[1] = 0x1FFF; //1-13 receivers selected
    setRgPacket.D.Rg32.Rg[2] = 0x08; //internal receiver address (0x08 FIFO)
    setRgPacket.D.Rg32.Rg[3] = 0x0A2; //Bit window position address
    setRgPacket.D.Rg32.Rg[4] = bitWindowPosition;
    setRgPacket.D.Rg32.Count = 5;
    setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;

    pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
}

void MainWindow::setReceiverBitWindowPosition(unsigned int recID, unsigned int bitWindowPosition){
    static tPkg setRgPacket;
    unsigned int mapID2Rec[] = {6,7,0,1,2,3,4,5};
    if (recID >= 1 && recID <= 8){
        setTypicalPacketPrefix(&setRgPacket, eRqst_Rdr_SetRgs32);
        setRgPacket.D.Rg32.Rg[0] = 0x0F; //receiver chip select address
        setRgPacket.D.Rg32.Rg[1] = 0x001 << mapID2Rec[recID - 1]; //receiver selected
        setRgPacket.D.Rg32.Rg[2] = 0x08; //internal receiver address (0x08 FIFO)
        setRgPacket.D.Rg32.Rg[3] = 0x0A2; //Bit window position address
        setRgPacket.D.Rg32.Rg[4] = bitWindowPosition;
        setRgPacket.D.Rg32.Count = 5;
        setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;

        pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
    }
}

void MainWindow::on_antennaGainSpin_valueChanged(int gain){
    static tPkg setRgPacket;
    setTypicalPacketPrefix(&setRgPacket, eRqst_Rdr_SetRgs32);
    setRgPacket.D.Rg32.Count = 5;
    setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;
    setRgPacket.D.Rg32.Rg[0] = 0x0F; //receiver chip select address
    setRgPacket.D.Rg32.Rg[1] = 0x001 << pAntennaReceiver[showAntennaA]; //receiver select
    setRgPacket.D.Rg32.Rg[2] = 0x08; //internal receiver address (0x08 FIFO)
    setRgPacket.D.Rg32.Rg[3] = 0x102 | (pAntennaReceiverChannel[showAntennaA] << 4); //virtual gain address for antenna in the receiver
    setRgPacket.D.Rg32.Rg[4] = gain; //

    ui->logText->append("DR " + QString::number(setRgPacket.D.Rg32.Rg[1]) + " A " + QString::number(pAntennaReceiverChannel[showAntennaA]) + " G " + QString::number(setRgPacket.D.Rg32.Rg[4]));
    pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
}

void MainWindow::on_SyncDriverClient_read(){
    char SyncDriverBuffer[8192];
    tSyncDriverPkg_Head* pSyncDriverHead =  reinterpret_cast<tSyncDriverPkg_Head*>(SyncDriverBuffer);
    tSyncDriverPkg_Dt* pSyncDriverData = reinterpret_cast<tSyncDriverPkg_Dt*>(SyncDriverBuffer + sizeof(tSyncDriverPkg_Head));
    if (static_cast<unsigned long>(pSyncDriverClient->bytesAvailable()) >= sizeof(tSyncDriverPkg_Head)){
        pSyncDriverClient->read(reinterpret_cast<char*>(pSyncDriverHead), sizeof(tPkg_Head));           //read header
        switch (pSyncDriverHead->Rqst){
            case eSyncDriverRqst_Rdr_GetCnfg:
                ui->logText->append("SyncDriver::eSyncDriverRqst_Rdr_GetCnfg::isRqstR" + QString::number(pSyncDriverHead->isRqstR));
                pSyncDriverClient->read(reinterpret_cast<char*>(pSyncDriverData), pSyncDriverHead->DtSz);           //read data
                ui->logText->append("FSetDuration " + QString::number(pSyncDriverData->Cfg.FSetDuration));
                ui->logText->append("PolarToggle " + QString::number(pSyncDriverData->Cfg.PolarToggle));
                ui->logText->append("PolarSet " + QString::number(pSyncDriverData->Cfg.PolarSet));
                ui->logText->append("HiTimeDuration " + QString::number(pSyncDriverData->Cfg.HiTimeDuration));
                ui->logText->append("LowTimeDuration " + QString::number(pSyncDriverData->Cfg.LowTimeDuration));
                ui->logText->append("NBand " + QString::number(pSyncDriverData->Cfg.NBand));
                ui->logText->append("NCycle " + QString::number(pSyncDriverData->Cfg.NCycle));
                ui->logText->append("NFSet " + QString::number(pSyncDriverData->Cfg.NFSet));
                ui->logText->append("Cntrl " + QString::number(pSyncDriverData->Cfg.Cntrl,16));
            break;
            case eSyncDriverRqst_Rdr_SetCnfg:
                ui->logText->append("SyncDriver::eSyncDriverRqst_Rdr_SetCnfg::isRqstR " + QString::number(pSyncDriverHead->isRqstR));
            break;
            default:
                ui->logText->append("SyncDriver::unknown");
        }
    }
}

void MainWindow::on_currentFrequencySpinBox_valueChanged(int newFreq){
    showFrequency = newFreq;
}

void MainWindow::on_yScaleScroll_valueChanged(int value){
    plotterYScale = value / 100.;
    ui->plotter->yAxis->setRange(-plotterYScale + plotterYOffset, plotterYScale + plotterYOffset);
    ui->antennaPlotter->yAxis->setRange(-plotterYScale + plotterYOffset, plotterYScale + plotterYOffset);
    ui->plotter->replot();
    ui->antennaPlotter->replot();
}

void MainWindow::on_yOffsetScroll_valueChanged(int value){
    plotterYOffset = value / 100.;
    ui->plotter->yAxis->setRange(-plotterYScale + plotterYOffset, plotterYScale + plotterYOffset);
    ui->antennaPlotter->yAxis->setRange(-plotterYScale + plotterYOffset, plotterYScale + plotterYOffset);
    ui->plotter->replot();
    ui->antennaPlotter->replot();
}

void MainWindow::on_xScaleScroll_valueChanged(int value){
    plotterXScale = value;
    if (packetNumber > plotterXScale)
        ui->plotter->xAxis->setRange(packetNumber - plotterXScale + plotterXOffset, packetNumber + plotterXOffset);
    else
        ui->plotter->xAxis->setRange(plotterXOffset, plotterXScale + plotterXOffset);
    ui->plotter->replot();
}

void MainWindow::on_xOffsetScroll_valueChanged(int value){
    plotterXOffset = value;
    if (packetNumber > plotterXScale)
        ui->plotter->xAxis->setRange(packetNumber - plotterXScale + plotterXOffset, packetNumber + plotterXOffset);
    else
        ui->plotter->xAxis->setRange(plotterXOffset, plotterXScale + plotterXOffset);
    ui->plotter->replot();
}

void MainWindow::setTypicalPacketPrefix(tPkg* pPacket, uint8_t request){
    pPacket->H.isPacked = 0;
    pPacket->H.HeadExtTp = 0x00;
    pPacket->H.Magic = 0x05;
    pPacket->H.Magic = 0x05;
    pPacket->H.Rqst = request;
}

void MainWindow::on_setTimeButton_clicked(){
    setCorrelatorTime();
}

void MainWindow::setCorrelatorTime(){
    tPkg setRgPacket;
    QDateTime curTime = QDateTime::currentDateTime();
    QString msg;
    QDateTime zeroMs = curTime;
    zeroMs.setTime(QTime(0,0));
    setRgPacket.H.isPacked = 0;
    setRgPacket.H.HeadExtTp = 0x00;
    setRgPacket.H.Magic = 0x05;

    setRgPacket.H.Rqst = eRqst_Rdr_SetRgs32;
    setRgPacket.D.Rg32.Count = 3;
    setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;
    setRgPacket.D.Rg32.Rg[0] = 2;
    setRgPacket.D.Rg32.Rg[1] = static_cast<uint32_t>((zeroMs.msecsTo(curTime)*1e5));
    setRgPacket.D.Rg32.Rg[2] = curTime.toTime_t();
    pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);

    msg.sprintf("timestamp ");
    ui->logText->append(msg + QString(curTime.toString("yyyyMMddThhmmss")));
    ui->logText->append(QString::asprintf("toTime_t %u", setRgPacket.D.Rg32.Rg[2]));
}

void MainWindow::on_initCorrelatorButton_clicked(bool checked){
    initCorrelator(checked);
}

void MainWindow::initCorrelator(bool acquire){
    static tPkg setRgPacket;
    QString msg;
    setRgPacket.H.isPacked = 0;
    setRgPacket.H.HeadExtTp = 0x00;
    setRgPacket.H.Magic = 0x05;

    if(acquire){
        firstPacketParsing = 1;
        parsingState = 0;
        metaDataReceived = false;

        if (!QDir("currentData/SRH1224/" + QDateTime::currentDateTime().toString("yyyyMMdd")).exists())
            QDir().mkdir("currentData/SRH1224/" + QDateTime::currentDateTime().toString("yyyyMMdd"));

        initDigitalReceivers();
//        initEphemeris();
//        initCoordinates();
//        setBitWindowPosition(10);
        setCorrelatorTime();
        setDelayTracking();
        setFringeStopping();
        setFrequencies();
        syncCorrelator(true);
        startCorrelator(true);
    } else {
        startCorrelator(false);
        syncCorrelator(false);
    }
}

void MainWindow::setFrequencies(){
    static tPkg setRgPacket;
    setTypicalPacketPrefix(&setRgPacket, eRqst_Rdr_SetRgs32);
    setRgPacket.D.Rg32.Rg[0] = 0x0000004;//start address
    setRgPacket.D.Rg32.Rg[1] = dataDelay;
    setRgPacket.D.Rg32.Rg[2] = dataDuration;//80 ms 2000000, 100 ms 2500000
    setRgPacket.D.Rg32.Rg[3] = 0;
    for (unsigned int i = 0;i < frequencyListSize;++i)
        setRgPacket.D.Rg32.Rg[i + 4] = frequencyList[i];

    setRgPacket.D.Rg32.Count = 4 + frequencyListSize;
    setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;
    pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
}

void MainWindow::syncCorrelator(bool start){
    static tPkg setRgPacket;
    setTypicalPacketPrefix(&setRgPacket, eRqst_Rdr_SetRgs32);
    setRgPacket.D.Rg32.Rg[0] = 1;
    if (start)
        setRgPacket.D.Rg32.Rg[1] = internalSync ? 0x2 : 0x1; // start sync
    else
        setRgPacket.D.Rg32.Rg[1] = 0; // stop sync
    setRgPacket.D.Rg32.Count = 2;
    setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;
    pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
}

void MainWindow::startCorrelator(bool start){
    static tPkg setRgPacket;
    if (start)
        setTypicalPacketPrefix(&setRgPacket, eRqst_SetStateProcessing); //DMA start
    else
        setTypicalPacketPrefix(&setRgPacket, eRqst_SetStateIdle); //DMA stop
    setRgPacket.H.DtSz = 0;
    pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head));
}

void MainWindow::on_delayTrackingButton_clicked(bool checked){
    delayTracking = checked;
}

void MainWindow::setDelayTracking(){
    static tPkg setRgPacket;
    setTypicalPacketPrefix(&setRgPacket, eRqst_Rdr_SetRgs32);
    setRgPacket.D.Rg32.Rg[0] = 0x0F; //receiver chip select address
    setRgPacket.D.Rg32.Rg[1] = 0x1FFF; //1-13 receivers selected
    setRgPacket.D.Rg32.Rg[2] = 0x08; //internal receiver address (0x08 FIFO)
    setRgPacket.D.Rg32.Rg[3] = 0x0B0;//control register
    setRgPacket.D.Rg32.Rg[4] = delayTracking;//delay tracking control

    setRgPacket.D.Rg32.Count = 5;
    setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;
    pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
    ui->logText->append("Delay tracking " + (delayTracking ? QString("ON") : QString("OFF")));
}

void MainWindow::on_fringeStoppingButton_clicked(bool checked){
    fringeStopping = checked;
}

void MainWindow::setFringeStopping(){
    static tPkg setRgPacket;
    setTypicalPacketPrefix(&setRgPacket, eRqst_Rdr_SetRgs32);
    setRgPacket.D.Rg32.Rg[0] = 0x0F; //receiver chip select address
    setRgPacket.D.Rg32.Rg[1] = 0x1FFF; //1-13 receivers selected
    setRgPacket.D.Rg32.Rg[2] = 0x08; //internal receiver address (0x08 FIFO)
    setRgPacket.D.Rg32.Rg[3] = 0x0A3; //Heterodin control
    if (fringeStopping)
        setRgPacket.D.Rg32.Rg[4] = 0x400;
    else
        setRgPacket.D.Rg32.Rg[4] = 0x000;
    setRgPacket.D.Rg32.Count = 5;
    setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;

    pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
    ui->logText->append("Fringe stopping " + (fringeStopping ? QString("ON") : QString("OFF")));
}

void MainWindow::on_oneBitCorrelationButton_clicked(bool checked){
    oneBitCorrelation = checked;
    static tPkg setRgPacket;
    setTypicalPacketPrefix(&setRgPacket, eRqst_Rdr_SetRgs32);
    setRgPacket.D.Rg32.Rg[0] = 0x0F; //
    setRgPacket.D.Rg32.Rg[1] = 0xFF; //
    setRgPacket.D.Rg32.Rg[2] = 0x08; //
    setRgPacket.D.Rg32.Rg[3] = 0x0A8;
    if (oneBitCorrelation)
        setRgPacket.D.Rg32.Rg[4] = 0;//
    else
        setRgPacket.D.Rg32.Rg[4] = quantizerStep | (qunatizerZeroLevel ? 0x80000000 : 0);//

    setRgPacket.D.Rg32.Count = 5;
    setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;
    pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
}

unsigned int MainWindow::visibilityIndexAsHV(unsigned int H, unsigned int V){
    unsigned int h = H > V ? H : V;
    unsigned int v = H > V ? V : H;
    return (h/4 - v/4) * 16 + (2 * 52 - v/4 + 1) * (v/4) *16 / 2 + v%4 * 4 + h%4;
}

void MainWindow::on_quantizerSpinBox_valueChanged(int newStep){
    quantizerStep = newStep;
}

void MainWindow::on_pushButton_clicked(bool checked){
    qunatizerZeroLevel = !checked;
}

void MainWindow::on_currentAntennaASpinBox_valueChanged(int newAntA){
    if (newAntA < amp01224Number){
        showAntennaA = newAntA;
        ui->plotter->graph(0)->setName(pAntennaName[showAntennaA]);
        ui->plotter->graph(2)->setName(pAntennaName[showAntennaA] + "-" + pAntennaName[showAntennaB] + " re");
        ui->plotter->graph(3)->setName(pAntennaName[showAntennaA] + "-" + pAntennaName[showAntennaB] + " im");
        ui->plotter->replot();
        replotAntennaMarkers();
        ui->antennaPlotter->replot();
    }
}

void MainWindow::on_currentAntennaBSpinBox_valueChanged(int newAntB){
    if (newAntB < amp01224Number){
        showAntennaB = newAntB;
        ui->plotter->graph(1)->setName(pAntennaName[showAntennaB]);
        ui->plotter->graph(2)->setName(pAntennaName[showAntennaA] + "-" + pAntennaName[showAntennaB] + " re");
        ui->plotter->graph(3)->setName(pAntennaName[showAntennaA] + "-" + pAntennaName[showAntennaB] + " im");
        ui->plotter->replot();
        replotAntennaMarkers();
        ui->antennaPlotter->replot();
    }
}
void MainWindow::initAntennaArrayLayoutPlotter(){
    ui->antennaArrayLayoutPlotter->xAxis->setRange(-400, 400);
    ui->antennaArrayLayoutPlotter->yAxis->setRange(-400, 20);

    ui->antennaArrayLayoutPlotter->addGraph();
    ui->antennaArrayLayoutPlotter->graph(0)->setPen(QPen(QColor(0,0,0),1));
    ui->antennaArrayLayoutPlotter->graph(0)->setLineStyle(QCPGraph::lsNone);
    ui->antennaArrayLayoutPlotter->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 2));

    ui->antennaArrayLayoutPlotter->addGraph();
    ui->antennaArrayLayoutPlotter->graph(1)->setPen(QPen(QColor(255,0,0),1));
    ui->antennaArrayLayoutPlotter->graph(1)->setLineStyle(QCPGraph::lsNone);
    ui->antennaArrayLayoutPlotter->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));

    ui->antennaArrayLayoutPlotter->addGraph();
    ui->antennaArrayLayoutPlotter->graph(2)->setPen(QPen(QColor(0,255,0),1));
    ui->antennaArrayLayoutPlotter->graph(2)->setLineStyle(QCPGraph::lsNone);
    ui->antennaArrayLayoutPlotter->graph(2)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 4));

    QVector<double> antennaX(amp01224Number);
    QVector<double> antennaY(amp01224Number);
    unsigned int i;

    for (i = 0;i < southAntennaNumber;++i){
        antennaX[i] = pSouthAntennaY[i] * 0.001;
        antennaY[i] = pSouthAntennaX[i] * 0.001;
    }
    for (i = 0;i < westAntennaNumber;++i){
        antennaX[i + southAntennaNumber] = pWestAntennaY[i] * 0.001;
        antennaY[i + southAntennaNumber] = pWestAntennaX[i] * 0.001;
    }
    antennaX[i + southAntennaNumber + westAntennaNumber] = centerAntennaY * 0.001;
    antennaY[i + southAntennaNumber + westAntennaNumber] = centerAntennaX * 0.001;
    for (i = 0;i < eastAntennaNumber;++i){
        antennaX[i + southAntennaNumber + westAntennaNumber + 1] = pEastAntennaY[i] * 0.001;
        antennaY[i + southAntennaNumber + westAntennaNumber + 1] = pEastAntennaX[i] * 0.001;
    }
    ui->antennaArrayLayoutPlotter->graph(0)->setData(antennaX,antennaY,amp01224Number);
}

void MainWindow::replotAntennaMarkers(){
    QVector<double> antennaAXmarker(1);
    QVector<double> antennaAYmarker(1);
    QVector<double> antennaBXmarker(1);
    QVector<double> antennaBYmarker(1);
    antennaAXmarker[0] =  antYColumn[showAntennaA] * 0.001;
    antennaAYmarker[0] =  antXColumn[showAntennaA] * 0.001;

    antennaBXmarker[0] =  antYColumn[showAntennaB] * 0.001;
    antennaBYmarker[0] =  antXColumn[showAntennaB] * 0.001;
    ui->antennaArrayLayoutPlotter->graph(1)->setData(antennaAXmarker, antennaAYmarker, 1);
    ui->antennaArrayLayoutPlotter->graph(2)->setData(antennaBXmarker, antennaBYmarker, 1);
    ui->antennaArrayLayoutPlotter->graph(1)->setName(pAntennaName[showAntennaA]);
    ui->antennaArrayLayoutPlotter->graph(2)->setName(pAntennaName[showAntennaB]);
    ui->antennaArrayLayoutPlotter->replot();
}

double MainWindow::TDHourAngle(double t, struct QSoldatFile::SSRTsolarDataRecord* pEphem){
    double  t1   = t - pEphem->culmination;
    return  pEphem->dHdt * t1 + pEphem->d2Hdt2/2. * t1*t1;
}

double MainWindow::TDDeclination(double t, struct QSoldatFile::SSRTsolarDataRecord* pEphem){
    double  t1   = t - pEphem->culmination;
    return  pEphem->declination + pEphem->dDdt * t1 + pEphem->d2Ddt2/2. * t1*t1;
}

double  MainWindow::delayForAntennaEast(double t, struct QSoldatFile::SSRTsolarDataRecord* pEphem, int A1){
    if (A1 >= 1 && A1 <= eastAntennaNumber){
        double cosP = sin(TDHourAngle(t,pEphem)) * cos(TDDeclination(t,pEphem));
        return -_BASE*A1*cosP / _C;
    }
    return 0.;
}

double  MainWindow::delayForAntennaWest(double t, struct QSoldatFile::SSRTsolarDataRecord* pEphem, int A1){
    if (A1 > 0 && A1 <= westAntennaNumber){
        double cosP = sin(TDHourAngle(t,pEphem)) * cos(TDDeclination(t,pEphem));
        return _BASE*A1*cosP / _C;
    }
    return 0.;
}

double  MainWindow::delayForAntennaSouth(double t, struct QSoldatFile::SSRTsolarDataRecord* pEphem, int A1){
    if (A1 >= 1 && A1 <= southAntennaNumber){
        double cosQ = cos(TDHourAngle(t,pEphem))*cos(TDDeclination(t,pEphem))*sin(_PHI) - sin(TDDeclination(t,pEphem))*cos(_PHI);
        return -_BASE*A1*cosQ / _C;
    }
    return 0.;
}

void MainWindow::on_calcAllDelaysButton_clicked(){
    QTime curTime = QTime::currentTime();
    double t = curTime.msecsSinceStartOfDay() * 0.001;
    const double constDelta = 20000.;
    double fEastDelayAsSrhDelta = -delayForAntennaEast(t, soldat.getCurrentRecord(), 1) * 1e10;
    double fWestDelayAsSrhDelta = -delayForAntennaWest(t, soldat.getCurrentRecord(), 1) * 1e10;
    double fSouthDelayAsSrhDelta = -delayForAntennaSouth(t, soldat.getCurrentRecord(), 1) * 1e10;
    int nEastDelayAsSrhDelta = fEastDelayAsSrhDelta > 0. ? (int)(fEastDelayAsSrhDelta + .5) : (int)(fEastDelayAsSrhDelta - 0.5);
    int nWestDelayAsSrhDelta = fWestDelayAsSrhDelta > 0. ? (int)(fWestDelayAsSrhDelta + .5) : (int)(fWestDelayAsSrhDelta - 0.5);
    int nSouthDelayAsSrhDelta = fSouthDelayAsSrhDelta > 0. ? (int)(fSouthDelayAsSrhDelta + .5) : (int)(fSouthDelayAsSrhDelta - 0.5);

    ui->logText->append("East delay " + QString::number(nEastDelayAsSrhDelta));
    ui->logText->append("West delay " + QString::number(nWestDelayAsSrhDelta));
    ui->logText->append("South delay " + QString::number(nSouthDelayAsSrhDelta));

    static tPkg setRgPacket;
    setTypicalPacketPrefix(&setRgPacket, eRqst_Rdr_SetRgs32);
    setRgPacket.D.Rg32.Count = 5;
    setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;
    setRgPacket.D.Rg32.Rg[0] = 0x0F; //receiver chip select address
    setRgPacket.D.Rg32.Rg[2] = 0x08; //internal receiver address (0x08 FIFO)

    for (unsigned int southAnt = 0;southAnt < southAntennaNumber;++southAnt){
        nSouthDelayAsSrhDelta = (int)(constDelta + fSouthDelayAsSrhDelta*(southAnt + 1) + .5);
        setRgPacket.D.Rg32.Rg[1] = 0x001 << pSouthAntennaReceiver[southAnt]; //receiver select
        setRgPacket.D.Rg32.Rg[3] = 0x100 | (pSouthAntennaChannel[southAnt] << 4); //virtual delay address for antenna in the receiver
        setRgPacket.D.Rg32.Rg[4] = nSouthDelayAsSrhDelta / 100; //coarse delay value
        pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
    }

    for (unsigned int westAnt = 0;westAnt < westAntennaNumber;++westAnt){
        nWestDelayAsSrhDelta = (int)(constDelta + fWestDelayAsSrhDelta*(westAnt + 1) + .5);
        setRgPacket.D.Rg32.Rg[1] = 0x001 << pWestAntennaReceiver[westAnt]; //receiver select
        setRgPacket.D.Rg32.Rg[3] = 0x100 | (pWestAntennaChannel[westAnt] << 4); //virtual delay address for antenna in the receiver
        setRgPacket.D.Rg32.Rg[4] = nWestDelayAsSrhDelta / 100; //coarse delay value
        pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
    }

    for (unsigned int eastAnt = 0;eastAnt < eastAntennaNumber;++eastAnt){
        nEastDelayAsSrhDelta = (int)(constDelta + fEastDelayAsSrhDelta*(eastAnt + 1) + .5);
        setRgPacket.D.Rg32.Rg[1] = 0x001 << pEastAntennaReceiver[eastAnt]; //receiver select
        setRgPacket.D.Rg32.Rg[3] = 0x100 | (pEastAntennaChannel[eastAnt] << 4); //virtual delay address for antenna in the receiver
        setRgPacket.D.Rg32.Rg[4] = nEastDelayAsSrhDelta / 100; //coarse delay value
        pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
    }

}

void MainWindow::on_showPolarizationButton_toggled(bool checked){
    showPolarization = checked;
    if (showPolarization == 0)
        ui->showPolarizationButton->setText("LCP");
    else
        ui->showPolarizationButton->setText("RCP");
}

void MainWindow::on_localOscillatorStartStop_clicked(bool start){
/*
    if (start){
        QStringList freqList;
        for (unsigned long i = 0;i < frequencyListSize;++i)
            freqList.append(QString::number(frequencyList[i]) + " kHz");
        ui->logText->append(localOscillator->startFrequencyList(freqList));
        ui->logText->append("LO start");
        ui->logText->append(localOscillator->getStatus());
    } else {
        ui->logText->append(localOscillator->stopFrequencyList());
        ui->logText->append("LO stop");
        ui->logText->append(localOscillator->getStatus());
    }
*/
}

void MainWindow::on_antennaADelaySpin_valueChanged(int arg1){
    antennaADelay = arg1;
    static tPkg setRgPacket;
    setTypicalPacketPrefix(&setRgPacket, eRqst_Rdr_SetRgs32);
    setRgPacket.D.Rg32.Count = 5;
    setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;
    setRgPacket.D.Rg32.Rg[0] = 0x0F; //receiver chip select address
    setRgPacket.D.Rg32.Rg[1] = 0x001 << pAntennaReceiver[showAntennaA]; //receiver select
    setRgPacket.D.Rg32.Rg[2] = 0x08; //internal receiver address (0x08 FIFO)

    setRgPacket.D.Rg32.Rg[3] = 0x100 | (pAntennaReceiverChannel[showAntennaA] << 4); //virtual delay address for antenna in the receiver
    setRgPacket.D.Rg32.Rg[4] = antennaADelay / 100; //coarse delay value
    pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);

    setRgPacket.D.Rg32.Rg[3] = 0x101 | (pAntennaReceiverChannel[showAntennaA] << 4); //virtual delay address for antenna in the receiver
    setRgPacket.D.Rg32.Rg[4] = antennaADelay % 100; //fine delay value
    pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
}

void MainWindow::on_antennaBDelaySpin_valueChanged(int arg1){
    antennaBDelay = arg1;
    static tPkg setRgPacket;
    setTypicalPacketPrefix(&setRgPacket, eRqst_Rdr_SetRgs32);
    setRgPacket.D.Rg32.Count = 5;
    setRgPacket.H.DtSz = (setRgPacket.D.Rg32.Count + 1)*4;
    setRgPacket.D.Rg32.Rg[0] = 0x0F; //receiver chip select address
    setRgPacket.D.Rg32.Rg[1] = 0x001 << pAntennaReceiver[showAntennaB]; //receiver select
    setRgPacket.D.Rg32.Rg[2] = 0x08; //internal receiver address (0x08 FIFO)

    setRgPacket.D.Rg32.Rg[3] = 0x100 | (pAntennaReceiverChannel[showAntennaB] << 4); //virtual delay address for antenna in the receiver
    setRgPacket.D.Rg32.Rg[4] = antennaBDelay / 100; //coarse delay value
    pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);

    setRgPacket.D.Rg32.Rg[3] = 0x101 | (pAntennaReceiverChannel[showAntennaA] << 4); //virtual delay address for antenna in the receiver
    setRgPacket.D.Rg32.Rg[4] = antennaBDelay % 100; //fine delay value
    pCorrelatorClient->write(reinterpret_cast<const char*>(&setRgPacket), sizeof(tPkg_Head) + setRgPacket.H.DtSz);
}
