#include "qg7m.h"

QG7M::QG7M(QString g7mIpAddress){
    viOpenDefaultRM(&defaultRsrcMngr);
    g7mName = "TCPIP::" + g7mIpAddress + "::8888::SOCKET::GNM";
    g7doesOperate = false;
}

QString QG7M::startFrequencyList(QStringList freqList){
    QString statusString;
    char buff[128];
    if (!g7doesOperate){
        status = viOpen(defaultRsrcMngr, (char*)g7mName.toStdString().data(), VI_EXCLUSIVE_LOCK,5000, &g7m);
        if (status == VI_SUCCESS){
/*
            viSetAttribute(g7m, VI_ATTR_TMO_VALUE, 20000);
            viPrintf(g7m,ViString("INIT:CONT OFF\n"));
            viPrintf(g7m, ViString("FREQ:MODE LIST\n"));
            viPrintf(g7m, ViString("LIST:FREQ 2800 MHz,3100 MHz,3500 MHz,4300 MHz,4900 MHz,5600 MHz\n"));
            viPrintf(g7m,ViString("TRIG:SEQ:SLOP NEG\n"));
            viPrintf(g7m,ViString("TRIG:SEQ:SOUR EXT\n"));
            viPrintf(g7m,ViString("LIST:MODE MAN\n"));
            viPrintf(g7m,ViString("POW:AMPL 10 DBM\n"));
            viPrintf(g7m,ViString("POW:MODE FIX\n"));
            viPrintf(g7m,ViString("OUTP:STAT ON\n"));
            viPrintf(g7m,ViString("INIT:CONT ON\n"));
*/
            viSetAttribute(g7m,VI_ATTR_TMO_VALUE, 20000);
            viPrintf(g7m,ViString("INIT:CONT OFF\n"));
            viQueryf(g7m,ViString("SYST:ERR?\n"), ViString("%T"), buff);
            statusString += "INIT:CONT" + QString(buff);

            viPrintf(g7m, ViString("FREQ:MODE LIST\n"));
            viQueryf(g7m,ViString("SYST:ERR?\n"), ViString("%T"), buff);
            statusString += "FREQ:MODE" + QString(buff);

            QString listCmd = "SOUR:LIST:FREQ ";
            for (int i = 0;i < freqList.size();++i){
                listCmd += freqList.at(i);
                if (i < freqList.size() - 1)
                    listCmd += ",";
                else
                    listCmd += "\n";
            }
    //        viPrintf(g7m, ViString(listCmd.toStdString().data()));
            viPrintf(g7m,ViString("LIST:FREQ 2800 MHz,3100 MHz,3500 MHz,4500 MHz,4900 MHz,5600 MHz\n"));
            viQueryf(g7m,ViString("SYST:ERR?\n"), ViString("%T"), buff);
            statusString += "LIST:FREQ" + QString(buff);


            viPrintf(g7m,ViString("TRIG:SEQ:SLOP NEG\n"));
            viQueryf(g7m,ViString("SYST:ERR?\n"), ViString("%T"), buff);
            statusString += "TRIG:SEQ:SLOP" + QString(buff);

            viPrintf(g7m,ViString("TRIG:SEQ:SOUR EXT\n"));
            viQueryf(g7m,ViString("SYST:ERR?\n"), ViString("%T"), buff);
            statusString += "TRIG:SEQ:SOUR" + QString(buff);

            viPrintf(g7m,ViString("LIST:MODE MAN\n"));
            viQueryf(g7m,ViString("SYST:ERR?\n"), ViString("%T"), buff);
            statusString += "LIST:MODE" + QString(buff);

            viPrintf(g7m,ViString("POW:AMPL 10 DBM\n"));
            viQueryf(g7m,ViString("SYST:ERR?\n"), ViString("%T"), buff);
            statusString += "POW:AMPL" + QString(buff);

            viPrintf(g7m,ViString("POW:MODE FIX\n"));
            viQueryf(g7m,ViString("SYST:ERR?\n"), ViString("%T"), buff);
            statusString += "POW:MODE" + QString(buff);

            viPrintf(g7m,ViString("OUTP:STAT ON\n"));
            viQueryf(g7m,ViString("SYST:ERR?\n"), ViString("%T"), buff);
            statusString += "OUTP:STAT" + QString(buff);

            viPrintf(g7m,ViString("INIT:CONT ON\n"));
            viQueryf(g7m,ViString("SYST:ERR?\n"), ViString("%T"), buff);
            statusString += "INIT:CONT ON " + QString(buff);

            g7doesOperate = true;
        }
    }
    return statusString;
}

QString QG7M::stopFrequencyList(){
    QString statusString;
    char buff[128];
    if (g7doesOperate){
        viPrintf(g7m,ViString("INIT:CONT OFF\n"));
        viQueryf(g7m,ViString("SYST:ERR?\n"), ViString("%T"), buff);
        statusString += "INIT:CONT OFF " + QString(buff);

        viPrintf(g7m,ViString("OUTP:STAT OFF\n"));
        viQueryf(g7m,ViString("SYST:ERR?\n"), ViString("%T"), buff);
        statusString += "OUTP:STAT OFF " + QString(buff);

        viClose(g7m);
        g7doesOperate = false;
    } else
        statusString = "g7doesOperate = false";
    return statusString;
}

QString QG7M::getStatus(){
    return g7mName + " " + QString::number(status, 16);
}
