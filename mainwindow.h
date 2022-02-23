#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QtNetwork>
#include <QString>
#include <valarray>
#include <complex>

#include "correlator.h"
#include "qsoldatfile.h"
#include "qg7m.h"

#include <CCfits>
using namespace CCfits;

struct SCorrVis{
    int32_t real;
    int32_t imag;
};

struct SAntennaPosition{
    long X;
    long Y;
    long Z;
};

struct SAntennaEquipmentID{
    long antennaName;
    long frontEndID;
    long feedID;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = NULL);
    ~MainWindow();

private slots:
    void on_connectButton_clicked(bool checked);

    void on_correlatorClient_connected();
    void on_correlatorClient_disconnected();

    void on_SyncDriverClient_connected();
    void on_SyncDriverClient_disconnected();
    void on_SyncDriverClient_read();

    void on_initReceiversButton_clicked();

    void on_SyncDriverConnect_clicked(bool checked);

    void on_currentFrequencySpinBox_valueChanged(int arg1);

    void on_yScaleScroll_valueChanged(int value);

    void on_yOffsetScroll_valueChanged(int value);

    void on_xScaleScroll_valueChanged(int value);

    void on_xOffsetScroll_valueChanged(int value);

    void on_setTimeButton_clicked();

    void on_initCorrelatorButton_clicked(bool checked);

    void on_correlatorClient_parse();

    void on_fringeStoppingButton_clicked(bool checked);

    void on_oneBitCorrelationButton_clicked(bool checked);

    void on_currentAntennaASpinBox_valueChanged(int arg1);

    void on_quantizerSpinBox_valueChanged(int arg1);

    void on_pushButton_clicked(bool checked);

    void setBitWindowPosition(unsigned int bitWindowPosition);

    void setReceiverBitWindowPosition(unsigned int recID, unsigned int bitWindowPosition);

    void initDigitalReceivers();

    void initEphemeris();

    void initCoordinates();

    void setCorrelatorTime();

    void initCorrelator(bool);

    void syncDriverStartStop(bool startStop);

    void setFringeStopping();

    void setDelayTracking();

    void setFrequencies();

    void syncCorrelator(bool start);

    void startCorrelator(bool start);

    void startSyncDriver();

    void on_currentAntennaBSpinBox_valueChanged(int arg1);

    void on_antennaGainSpin_valueChanged(int arg1);

    void on_calcAllDelaysButton_clicked();

    void on_delayTrackingButton_clicked(bool checked);

    void on_showPolarizationButton_toggled(bool checked);

    void on_SyncDriverGetConfigButton_clicked();

    void on_SyncDriverSetConfigButton_clicked(bool checked);

    void on_localOscillatorStartStop_clicked(bool checked);

    void on_antennaADelaySpin_valueChanged(int arg1);

    void on_antennaBDelaySpin_valueChanged(int arg1);

private:
    Ui::MainWindow *ui;
    QTcpSocket* pCorrelatorClient;
    QTcpSocket* pSyncDriverClient;
    QString correlatorIP;
    QString localOscillatorIP;
    int correlatorPort;
    QString SyncDriverIP;
    int SyncDriverPort;
    unsigned long packetNumber;
    unsigned long lcpFullPacketNumber;
    unsigned long rcpFullPacketNumber;
    qint64 packetSize;
    unsigned char* dataPacket;
    int parsingState;
    int firstPacketParsing;
    char correlatorRawBuffer[200000];
    int antennaADelay;
    int antennaBDelay;
    bool metaDataReceived;

    unsigned long fullPacketsInFits;
    unsigned long fitsNumber;
    unsigned long numberOfFitsVisibilities;
    unsigned long  numberOfShowVisibilities;
    unsigned long  numberOfFitsAmplitudes;
    unsigned long currentFrequency;
    uint32_t currentPolarization;
    unsigned long showFrequency;
    unsigned long showPolarization;
    unsigned long showAntennaA;
    unsigned long showAntennaB;
    unsigned long frequencyListSize;
    unsigned int* frequencyList;
    unsigned int dataDelay;
    unsigned int dataDuration;
    unsigned int maxVisValue;
    unsigned int frequencyDelay;
    bool internalSync;
    unsigned int quantizerStep;
    bool qunatizerZeroLevel;
    bool oneBitCorrelation;
    bool delayTracking;
    bool fringeStopping;
    bool autoStart;

    float plotterXScale;
    float plotterXOffset;
    float plotterYScale;
    float plotterYOffset;

    std::vector<string> antNameColumn;
    std::vector<string> antNameIndexColumn;

    std::vector<int> antIndexColumn;
    std::vector<int> antXColumn;
    std::vector<int> antYColumn;
    std::vector<int> antZColumn;
    std::vector<int> antFrontEndColumn;
    std::vector<int> antFeedColumn;
    std::vector<float> antDiameterColumn;

    std::vector<int> antAColumn;
    std::vector<int> antBColumn;

    std::vector<float> freqColumn;
    std::vector<std::valarray<double> > timeColumn;
    std::vector<std::valarray<int64_t > > lcpAmpColumn;
    std::vector<std::valarray<int64_t > > rcpAmpColumn;
    std::vector<std::valarray< complex<float> > > lcpVisColumn;
    std::vector<std::valarray< complex<float> > > rcpVisColumn;
    QString fitsPath;
    void addKey2FitsHeader(QString key, QString value, QString comment, FITS* pFits);
    void addKey2FitsHeader(QString key, int value, FITS* pFits);

    QSoldatFile soldat;

    void writeCurrentFits();
    void setTypicalPacketPrefix(tPkg*, uint8_t);

    unsigned int* pAntennaReceiver;
    unsigned int* pAntennaReceiverChannel;
    unsigned int visibilityIndexAsHV(unsigned int, unsigned int);

    double visScale;
    double ampScale;

    unsigned int vis01224Number;
    unsigned int southEastWestVisNumber;
    unsigned int southVisNumber;
    unsigned int eastWestVisNumber;
    unsigned int* pVisIndToCorrPacketInd;
    unsigned int* pVisIndToFitsInd;
    int* pVisibilitySign;
    unsigned int amp01224Number;
    unsigned int* pAmpIndToCorrPacketInd;
    unsigned int* pAmpIndToFitsInd;
    QString* pAntennaName;
    unsigned int* pAntennaDelay;

    unsigned int southAntennaNumber;
    QString* pSouthAntennaName;
    unsigned int* pSouthAntennaReceiver;
    unsigned int* pSouthAntennaChannel;
    unsigned int* pSouthAntennaFrontEndID;
    unsigned int* pSouthAntennaFeedID;
    float* pSouthAntennaDiameter;
    unsigned int* pSouthAntennaDelay;
    int* pSouthAntennaX;
    int* pSouthAntennaY;
    int* pSouthAntennaZ;
    unsigned int* pSouthAntennaIndex;

    unsigned int eastAntennaNumber;
    QString* pEastAntennaName;
    unsigned int* pEastAntennaReceiver;
    unsigned int* pEastAntennaChannel;
    unsigned int* pEastAntennaFrontEndID;
    unsigned int* pEastAntennaFeedID;
    float* pEastAntennaDiameter;
    unsigned int* pEastAntennaDelay;
    int* pEastAntennaX;
    int* pEastAntennaY;
    int* pEastAntennaZ;
    unsigned int* pEastAntennaIndex;

    unsigned int westAntennaNumber;
    QString* pWestAntennaName;
    unsigned int* pWestAntennaReceiver;
    unsigned int* pWestAntennaChannel;
    unsigned int* pWestAntennaFrontEndID;
    unsigned int* pWestAntennaFeedID;
    float* pWestAntennaDiameter;
    unsigned int* pWestAntennaDelay;
    int* pWestAntennaX;
    int* pWestAntennaY;
    int* pWestAntennaZ;
    unsigned int* pWestAntennaIndex;

    QString centerAntennaName;
    unsigned int centerAntennaReceiver;
    unsigned int centerAntennaChannel;
    unsigned int centerAntennaFrontEndID;
    unsigned int centerAntennaFeedID;
    float centerAntennaDiameter;
    unsigned int centerAntennaDelay;
    int centerAntennaX;
    int centerAntennaY;
    int centerAntennaZ;
    unsigned int centerAntennaIndex;

    unsigned int eastWestAntennaNumber;
    unsigned int* pEastWestAntennaReceiver;
    unsigned int* pEastWestAntennaChannel;
    QString* pVis01224AntennaNameA;
    QString* pVis01224AntennaNameB;
    QString* pEastWestAntennaName;

    QMap<QString, unsigned int> antName2indexMap;

    void initAntennaArrayLayoutPlotter();
    void replotAntennaMarkers();

    const double _BASE = 4.9;
    const double _C = 2.99793e8;
    const double _PHI = 0.903338787600965;

    double TDHourAngle(double t, struct QSoldatFile::SSRTsolarDataRecord* pEphem);
    double TDDeclination(double t, struct QSoldatFile::SSRTsolarDataRecord* pEphem);
    double  delayForAntennaEast(double t, struct QSoldatFile::SSRTsolarDataRecord* pEphem, int A1);
    double  delayForAntennaWest(double t, struct QSoldatFile::SSRTsolarDataRecord* pEphem, int A1);
    double  delayForAntennaSouth(double t, struct QSoldatFile::SSRTsolarDataRecord* pEphem, int A1);

    tSyncDriverConfigure SyncDriverConfig;
    QG7M* localOscillator;
};

#endif // MAINWINDOW_H
