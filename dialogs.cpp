/*
 *  Copyright (C) 2013-2014 Ofer Kashayov <oferkv@live.com>
 *  This file is part of Phototonic Image Viewer.
 *
 *  Phototonic is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Phototonic is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Phototonic.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "dialogs.h"
#include "global.h"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv/cxcore.h"
#include "opencv/cv.h"

using namespace std;
using namespace cv;

int updateFlag = 0;
int peoNum = 0;
float xPos[10], yPos[10];

QString room1_str, room2_str, room3_str, room4_str;
QString room1_client_str, room2_client_str, room3_client_str, room4_client_str;

#define ROOM1_STR "00:0f:e2:78:cf:00"
#define ROOM2_STR "00:0f:e2:78:cf:01"
#define ROOM3_STR "00:0f:e2:78:cf:02"
#define ROOM4_STR "00:0f:e2:78:cf:03"
#define ROOM5_STR "00:0c:43:30:62:01"
#define ROOM6_STR "d8:42:ac:41:55:1e"
#define ROOM7_STR ""
#define ROOM8_STR ""

CpMvDialog::CpMvDialog(QWidget *parent) : QDialog(parent)
{
	abortOp = false;

    opLabel = new QLabel("");
    
    cancelButton = new QPushButton(tr("Cancel"));
    cancelButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(abort()));

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(opLabel);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(buttonsLayout, Qt::AlignRight);
    setLayout(mainLayout);
}

void CpMvDialog::abort()
{
	abortOp = true;
}

static QString autoRename(QString &destDir, QString &currFile)
{
	int extSep = currFile.lastIndexOf(".");
	QString nameOnly = currFile.left(extSep);
	QString extOnly = currFile.right(currFile.size() - extSep - 1);
	QString newFile;

	int idx = 1;

	do
	{
		newFile = QString(nameOnly + "_copy_%1." + extOnly).arg(idx);
		++idx;
	} 
	while (idx && (QFile::exists(destDir + QDir::separator() + newFile)));

	return newFile;
}

int cpMvFile(bool isCopy, QString &srcFile, QString &srcPath, QString &dstPath, QString &dstDir)
{
	int res;
	
	if (isCopy)
		res = QFile::copy(srcPath, dstPath);
	else
		res = QFile::rename(srcPath, dstPath);
		
	if (!res && QFile::exists(dstPath))
	{
		QString newName = autoRename(dstDir, srcFile);
		QString newDestPath = dstDir + QDir::separator() + newName;
		
		if (isCopy)
			res = QFile::copy(srcPath, newDestPath);
		else
			res = QFile::rename(srcPath, newDestPath);
		dstPath = newDestPath;
	}

	return res;
}

void CpMvDialog::exec(ThumbView *thumbView, QString &destDir, bool pasteInCurrDir)
{
	int res = 0;
	QString sourceFile;
	QFileInfo fileInfo;
	QString currFile;
	QString destFile;

	show();

	if (pasteInCurrDir)
	{
		int tn = 0;
		for (tn = 0; tn < GData::copyCutFileList.size(); ++tn)
		{
			sourceFile = GData::copyCutFileList[tn];
			fileInfo = QFileInfo(sourceFile);
			currFile = fileInfo.fileName();
			destFile = destDir + QDir::separator() + currFile;

			opLabel->setText((GData::copyOp? tr("Copying "):tr("Moving ")) + sourceFile + tr(" to ") + destFile);
			QApplication::processEvents();

			res = cpMvFile(GData::copyOp, currFile, sourceFile, destFile, destDir);

			if (!res || abortOp)
			{
				break;
			}
			else 
			{
				GData::copyCutFileList[tn] = destFile;
			}
		}
	}
	else
	{
		QList<int> rowList;
		int tn = 0;
		for (tn = GData::copyCutIdxList.size() - 1; tn >= 0 ; --tn)
		{
			sourceFile = thumbView->thumbViewModel->item(GData::copyCutIdxList[tn].row())->
																data(thumbView->FileNameRole).toString();
			fileInfo = QFileInfo(sourceFile);
			currFile = fileInfo.fileName();
			destFile = destDir + QDir::separator() + currFile;

			opLabel->setText((GData::copyOp? tr("Copying ") : 
												tr("Moving ")) + sourceFile + tr(" to ") + destFile);
			QApplication::processEvents();

			res = cpMvFile(GData::copyOp, currFile, sourceFile, destFile, destDir);

			if (!res || abortOp)
			{
				break;
			}

			rowList.append(GData::copyCutIdxList[tn].row());
		}

		if (!GData::copyOp)
		{
			qSort(rowList);
			for (int t = rowList.size() - 1; t >= 0; --t)
				thumbView->thumbViewModel->removeRow(rowList.at(t));
		}
	}

	close();	
}

KeyGrabLineEdit::KeyGrabLineEdit(QWidget *parent, QComboBox *combo) : QLineEdit(parent)
{
	keysCombo = combo;
	setClearButtonEnabled(true);
}

void KeyGrabLineEdit::keyPressEvent(QKeyEvent *e)
{
	QString keySeqText;
	QString keyText("");
	QString modifierText("");

	if (e->modifiers() & Qt::ShiftModifier)
		modifierText += "Shift+";		
	if (e->modifiers() & Qt::ControlModifier)
		modifierText += "Ctrl+";
	if (e->modifiers() & Qt::AltModifier)
		modifierText += "Alt+";

	if ((e->key() >= Qt::Key_Shift &&  e->key() <= Qt::Key_ScrollLock) || 
			(e->key() >= Qt::Key_Super_L &&  e->key() <= Qt::Key_Direction_R) ||
			e->key() == Qt::Key_AltGr ||
			e->key() < 0) 
		return;

	keyText = QKeySequence(e->key()).toString();
	keySeqText = modifierText + keyText;

	QMapIterator<QString, QAction *> it(GData::actionKeys);
	while (it.hasNext())
	{
		it.next();
		if (it.value()->shortcut().toString() == keySeqText)
		{
			QMessageBox msgBox;
			msgBox.warning(this, tr("Set shortcut"), tr("Already assigned to \"")
																		+ it.key() + tr("\" action"));
			return;
		}
	}
	
	setText(keySeqText);
	GData::actionKeys.value(keysCombo->currentText())->setShortcut(QKeySequence(keySeqText));
}

void KeyGrabLineEdit::clearShortcut()
{
	if (text() == "")
		GData::actionKeys.value(keysCombo->currentText())->setShortcut(QKeySequence(""));
}

void SettingsDialog::setActionKeyText(const QString &text)
{
	keyLine->setText(GData::actionKeys.value(text)->shortcut().toString());
}

ElocDetectDialog::ElocDetectDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("ElocDetect"));

	room1_str = room2_str = room3_str = room4_str = "";
	room1_client_str = room2_client_str = room3_client_str = room4_client_str = "";

//	room1_str = room2_str = room3_str = room4_str = "";
//	room1_client_str = room2_client_str = room3_client_str = room4_client_str = "";
//	room1_str = "00:0f:e2:78:cf:00";
//	room2_str = "00:0f:e2:78:cf:01";
//	room3_str = "00:0f:e2:78:cf:02";
//	room4_str = "00:0f:e2:78:cf:03";
//	room1_str = ROOM1_STR;
//	room2_str = ROOM2_STR;
//	room3_str = ROOM3_STR;
//	room4_str = ROOM4_STR;
//	room5_str = ROOM5_STR;
//	room6_str = ROOM6_STR;

	int height = parent->size().height() - 50;
	if (height > 800)
		height = 800;
	resize(600, height);

	QVBoxLayout *HeadVbox = new QVBoxLayout;

	QHBoxLayout *firstHbox = new QHBoxLayout;
	QHBoxLayout *secondHbox = new QHBoxLayout;
	QHBoxLayout *thirdHbox = new QHBoxLayout;
	QHBoxLayout *fourthHbox = new QHBoxLayout;

//	firstRouterEdit = new QLineEdit;
//	secondRouterEdit = new QLineEdit;
//	thirdRouterEdit = new QLineEdit;
//	fourthRouterEdit = new QLineEdit;

	firstRouterBox = new QComboBox(this);
	secondRouterBox = new QComboBox(this);
	thirdRouterBox = new QComboBox(this);
	fourthRouterBox = new QComboBox(this);

	firstRouterBox->addItem(ROOM1_STR);
	firstRouterBox->addItem(ROOM2_STR);
	firstRouterBox->addItem(ROOM3_STR);
	firstRouterBox->addItem(ROOM4_STR);
	firstRouterBox->addItem(ROOM5_STR);
	firstRouterBox->addItem(ROOM6_STR);

	secondRouterBox->addItem(ROOM1_STR);
	secondRouterBox->addItem(ROOM2_STR);
	secondRouterBox->addItem(ROOM3_STR);
	secondRouterBox->addItem(ROOM4_STR);
	secondRouterBox->addItem(ROOM5_STR);
	secondRouterBox->addItem(ROOM6_STR);

	thirdRouterBox->addItem(ROOM1_STR);
	thirdRouterBox->addItem(ROOM2_STR);
	thirdRouterBox->addItem(ROOM3_STR);
	thirdRouterBox->addItem(ROOM4_STR);
	thirdRouterBox->addItem(ROOM5_STR);
	thirdRouterBox->addItem(ROOM6_STR);

	fourthRouterBox->addItem(ROOM1_STR);
	fourthRouterBox->addItem(ROOM2_STR);
	fourthRouterBox->addItem(ROOM3_STR);
	fourthRouterBox->addItem(ROOM4_STR);
	fourthRouterBox->addItem(ROOM5_STR);
	fourthRouterBox->addItem(ROOM6_STR);

	connect(firstRouterBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged1(int)));
	connect(secondRouterBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged2(int)));
	connect(thirdRouterBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged3(int)));
	connect(fourthRouterBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onChanged4(int)));

//	firstRouterEdit->setText(room1_str);
//	secondRouterEdit->setText(room2_str);
//	thirdRouterEdit->setText(room3_str);
//	fourthRouterEdit->setText(room4_str);

//	firstClientEdit = new QLineEdit;
	firstClientEdit = new QTextEdit;
	secondClientEdit = new QTextEdit;
	thirdClientEdit = new QTextEdit;
	fourthClientEdit = new QTextEdit;

	QPushButton *elocDetectButton = new QPushButton(tr("Eloc Detect"));
	connect(elocDetectButton, SIGNAL(clicked()), this, SLOT(elocDetect()));

//	QString str = "188:30:8a:3f:3f:a300:0f:e2:78:cf:0100:0f:e2:78:cf:03";
//	firstRouterEdit->setText(str.mid(1, 17));
//	firstClientEdit->setText(str.mid(18, 17) + "\n" + str.mid(35, 17));
//	firstHbox->addWidget(firstRouterEdit);
	firstHbox->addWidget(firstRouterBox);
	firstHbox->addWidget(firstClientEdit);

//	secondHbox->addWidget(secondRouterEdit);
	secondHbox->addWidget(secondRouterBox);
	secondHbox->addWidget(secondClientEdit);

//	thirdHbox->addWidget(thirdRouterEdit);
	thirdHbox->addWidget(thirdRouterBox);
	thirdHbox->addWidget(thirdClientEdit);

//	fourthHbox->addWidget(fourthRouterEdit);
	fourthHbox->addWidget(fourthRouterBox);
	fourthHbox->addWidget(fourthClientEdit);

	HeadVbox->addLayout(firstHbox);
	HeadVbox->addLayout(secondHbox);
	HeadVbox->addLayout(thirdHbox);
	HeadVbox->addLayout(fourthHbox);
	HeadVbox->addWidget(elocDetectButton);
	setLayout(HeadVbox);
}

/* added by LTC */
AutoDetectDialog::AutoDetectDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("AutoDetect"));

//	保持比后面父窗口小，上下留出空间
	int height = parent->size().height() - 50;
	if (height > 800)
		height = 800;
	resize(600, height);

	QHBoxLayout *timeRangeHbox = new QHBoxLayout;

	QLabel *startTimeLabel = new QLabel(tr("start :"));
	startTimeLabel->setMaximumWidth(50);
	startTimeEdit = new QLineEdit;
	startTimeEdit->setClearButtonEnabled(true);
	startTimeEdit->setMinimumWidth(100);
	startTimeEdit->setMaximumWidth(150);

	QLabel *endTimeLabel = new QLabel(tr("end :"));
	endTimeLabel->setMaximumWidth(50);
	endTimeEdit = new QLineEdit;
	endTimeEdit->setClearButtonEnabled(true);
	endTimeEdit->setMinimumWidth(100);
	endTimeEdit->setMaximumWidth(150);

	QPushButton *autoDetectButton = new QPushButton(tr("Auto Detect"));
	connect(autoDetectButton, SIGNAL(clicked()), this, SLOT(autoDetect()));

	timeRangeHbox->addWidget(startTimeLabel, 0, Qt::AlignLeft);
	timeRangeHbox->addWidget(startTimeEdit);
	timeRangeHbox->addStretch(1);
	timeRangeHbox->addWidget(endTimeLabel, 0, Qt::AlignLeft);
	timeRangeHbox->addWidget(endTimeEdit);
	timeRangeHbox->addStretch(1);
	timeRangeHbox->addWidget(autoDetectButton);

	QLabel *eLocLabel = new QLabel(tr("E-signal Location:"));
	eLocEdit = new QLineEdit;
	eLocEdit->setClearButtonEnabled(true);
	eLocEdit->setMinimumWidth(400);
	eLocEdit->setMaximumWidth(450);

	QLabel *vLocLabel = new QLabel(tr("V-signal Location:"));
	vLocEdit = new QLineEdit;
	vLocEdit->setClearButtonEnabled(true);
	vLocEdit->setMinimumWidth(400);
	vLocEdit->setMaximumWidth(450);

	QHBoxLayout *eLocLayout = new QHBoxLayout;
	eLocLayout->addWidget(eLocLabel);
	eLocLayout->addWidget(eLocEdit);
	QHBoxLayout *vLocLayout = new QHBoxLayout;
	vLocLayout->addWidget(vLocLabel);
	vLocLayout->addWidget(vLocEdit);

	QLabel *evLocLabel = new QLabel(tr("E-V Location:"));
	evLocEdit = new QLineEdit;
	evLocEdit->setClearButtonEnabled(true);
	evLocEdit->setMinimumWidth(400);
	evLocEdit->setMaximumWidth(450);

	QHBoxLayout *comLocLayout = new QHBoxLayout;
	comLocLayout->addWidget(evLocLabel);
	comLocLayout->addWidget(evLocEdit);

	myLabel *showImageLabel = new myLabel;

	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addLayout(timeRangeHbox);
	mainVbox->addLayout(eLocLayout);
	mainVbox->addLayout(vLocLayout);
	mainVbox->addLayout(comLocLayout);
	mainVbox->addWidget(showImageLabel);
	setLayout(mainVbox);
}

/* added by LTC */
myLabel::myLabel()
{
	resize(400,400);
	setWindowTitle(tr("Paint Demo"));
}

/* added by LTC */
void myLabel::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	painter.setPen(QPen(Qt::blue, 3));
	painter.setBrush(QBrush(Qt::black, Qt::CrossPattern));
	painter.drawRect(10, 10, 400, 400);
	if (updateFlag) {
		painter.setPen(QPen(Qt::red, 8));
		for (int i = 0; i < peoNum; i++) {
			if (xPos[i] >= 0 && yPos[i] >= 0) {
				painter.drawPoint((int)(xPos[i] * 100 + 10), (int)(yPos[i] * 100 + 10));
printf("LTC print drawPoint num %d, x is %d, y is %d, peoNum is %d\n", i, (int)(xPos[i] * 100 + 10), (int)(yPos[i] * 100 + 10), peoNum);
			}
		}
		for (int i = 0; i <= peoNum; i++) {
			xPos[i] = 0;
			yPos[i] = 0;
		}
		peoNum = 0;
	}
}

/* added by LTC */
void myLabel::drawPoint(float x, float y)
{
	QPainter painter(this);
	painter.setPen(QPen(Qt::blue, 3));
	painter.drawPoint(x, y);
}

/* Added by LTC */
QElocClient::QElocClient(QObject *parent)
	: QTcpSocket(parent)
{
	connect(this, SIGNAL(readyRead()), this, SLOT(readClient()));
	connect(this, SIGNAL(disconnected()), this, SLOT(deleteLater()));
}

/* Added by LTC */
QElocServer::QElocServer(QObject *parent)
	: QTcpServer(parent)
{
//	listen(QHostAddress::Any, 9999);
}

/* Added by LTC */
void QElocServer::incomingConnection(qintptr socketDescriptor)
{
printf("LTC print before new client socket\n");
	QElocClient *socket = new QElocClient(this);
	connect(socket, SIGNAL(readOver()), this, SLOT(readOverToDialog()));
printf("LTC print after new client socket\n");
	if (!socket->setSocketDescriptor(socketDescriptor)) {
printf("LTC print error new client socket\n");
		emit error(socket->error());
		return;
	}
}

void QElocServer::readOverToDialog()
{
	emit readOver2();
}

/* added by LTC*/
void ElocDetectDialog::elocDetect()
{
//	start_time = time(NULL);

	QTimer *clearTimer = new QTimer(this);
	connect(clearTimer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
	clearTimer->start(2000);

#if 0
	server = new QTcpServer();
	server->listen(QHostAddress::Any, 9999);
	connect(server, SIGNAL(newConnection()), this, SLOT(acceptConnection()));
#endif
	QElocServer *server = new QElocServer(this);
	connect(server, SIGNAL(readOver2()), this, SLOT(setClient()));
	if (!server->listen(QHostAddress::Any, 9999)) {
		QMessageBox::critical(this, tr("Eloc Server"),
				tr("Unable To start the server: %d")
				.arg(server->errorString()));
		server->close();
	}
#if 0
	QElocServer server;
	if (!server.listen(QHostAddress::Any, 9999)) {
		QMessageBox::critical(this, tr("Eloc Server"),
				tr("Unable To start the server: %d")
				.arg(server.errorString()));
		server.close();
	}
#endif
printf("LTC print after connect server\n");
}
#if 0

void ElocDetectDialog::acceptConnection()
{
	clientConnection = server->nextPendingConnection();
	connect(clientConnection, SIGNAL(readyRead()), this, SLOT(readClient()));
printf("LTC print after connect client\n");
}
#endif

void ElocDetectDialog::onChanged1(int index)
{
	switch (index) {
		case 0:{
			room1_str = ROOM1_STR;
			cout << "room0:" << room1_str.toStdString() << endl;
			break;
		}
		case 1:{
			room1_str = ROOM2_STR;
			cout << "room1:" << room1_str.toStdString() << endl;
			break;
		}
		case 2:{
			room1_str = ROOM3_STR;
			cout << "room2:" << room1_str.toStdString() << endl;
			break;
		}
		case 3:{
			room1_str = ROOM4_STR;
			cout << "room3:" << room1_str.toStdString() << endl;
			break;
		}
		case 4:{
			room1_str = ROOM5_STR;
			cout << "room4:" << room1_str.toStdString() << endl;
			break;
		}
		case 5:{
			room1_str = ROOM6_STR;
			cout << "room5:" << room1_str.toStdString() << endl;
			break;
		}
	}
}

void ElocDetectDialog::onChanged2(int index)
{
	switch (index) {
		case 0:{
			room2_str = ROOM1_STR;
			cout << "room0:" << room1_str.toStdString() << endl;
			break;
		}
		case 1:{
			room2_str = ROOM2_STR;
			cout << "room1:" << room1_str.toStdString() << endl;
			break;
		}
		case 2:{
			room2_str = ROOM3_STR;
			cout << "room2:" << room1_str.toStdString() << endl;
			break;
		}
		case 3:{
			room2_str = ROOM4_STR;
			cout << "room3:" << room1_str.toStdString() << endl;
			break;
		}
		case 4:{
			room2_str = ROOM5_STR;
			cout << "room4:" << room1_str.toStdString() << endl;
			break;
		}
		case 5:{
			room2_str = ROOM6_STR;
			cout << "room5:" << room1_str.toStdString() << endl;
			break;
		}
	}
}

void ElocDetectDialog::onChanged3(int index)
{
	switch (index) {
		case 0:{
			room3_str = ROOM1_STR;
			cout << "room0:" << room1_str.toStdString() << endl;
			break;
		}
		case 1:{
			room3_str = ROOM2_STR;
			cout << "room1:" << room1_str.toStdString() << endl;
			break;
		}
		case 2:{
			room3_str = ROOM3_STR;
			cout << "room2:" << room1_str.toStdString() << endl;
			break;
		}
		case 3:{
			room3_str = ROOM4_STR;
			cout << "room3:" << room1_str.toStdString() << endl;
			break;
		}
		case 4:{
			room3_str = ROOM5_STR;
			cout << "room4:" << room1_str.toStdString() << endl;
			break;
		}
		case 5:{
			room3_str = ROOM6_STR;
			cout << "room5:" << room1_str.toStdString() << endl;
			break;
		}
	}
}

void ElocDetectDialog::onChanged4(int index)
{
	switch (index) {
		case 0:{
			room4_str = ROOM1_STR;
			cout << "room0:" << room1_str.toStdString() << endl;
			break;
		}
		case 1:{
			room4_str = ROOM2_STR;
			cout << "room1:" << room1_str.toStdString() << endl;
			break;
		}
		case 2:{
			room4_str = ROOM3_STR;
			cout << "room2:" << room1_str.toStdString() << endl;
			break;
		}
		case 3:{
			room4_str = ROOM4_STR;
			cout << "room3:" << room1_str.toStdString() << endl;
			break;
		}
		case 4:{
			room4_str = ROOM5_STR;
			cout << "room4:" << room1_str.toStdString() << endl;
			break;
		}
		case 5:{
			room4_str = ROOM6_STR;
			cout << "room5:" << room1_str.toStdString() << endl;
			break;
		}
	}
}

void ElocDetectDialog::timerUpdate()
{
cout << "timer update " << endl;
	room1_client_str = room2_client_str = room3_client_str = room4_client_str = "";
	setClient();
}
#if 1

void ElocDetectDialog::setClient()
{
printf("set Client for dialog\n");
	firstClientEdit->setText(room1_client_str);
	secondClientEdit->setText(room2_client_str);
	thirdClientEdit->setText(room3_client_str);
	fourthClientEdit->setText(room4_client_str);

	update();
}
#endif

void QElocClient::readClient()
{
cout << "socket recieve :" << endl;
#if 0
	QString str = clientConnection->readAll();
	QString str_router = str.mid(18, 17);
	QString str_client = str.mid(1, 17);
	if (str_router == room1_str)
		if (room1_client_str.indexOf(str_client) < 0)
			room1_client_str += "\n" + str_client;
	if (str_router == room2_str)
		if (room2_client_str.indexOf(str_client) < 0)
			room2_client_str += "\n" + str_client;
	if (str_router == room3_str)
		if (room3_client_str.indexOf(str_client) < 0)
			room3_client_str += "\n" + str_client;
	if (str_router == room4_str)
		if (room4_client_str.indexOf(str_client) < 0)
			room4_client_str += "\n" + str_client;
cout << "socket recieve :" << (str.toStdString()) << endl;
	setClient();
#endif
	QString str = "";
	QTextStream in(this);
//	in.setVersion(QDataStream::Qt_5_3);
	in >> str;
	QString str_router = str.mid(18, 17);
	QString str_client = str.mid(1, 17);
	if (str_router == room1_str)
		if (room1_client_str.indexOf(str_client) < 0)
			room1_client_str += "\n" + str_client;
	if (str_router == room2_str)
		if (room2_client_str.indexOf(str_client) < 0)
			room2_client_str += "\n" + str_client;
	if (str_router == room3_str)
		if (room3_client_str.indexOf(str_client) < 0)
			room3_client_str += "\n" + str_client;
	if (str_router == room4_str)
		if (room4_client_str.indexOf(str_client) < 0)
			room4_client_str += "\n" + str_client;
cout << "socket recieve :" << (str.toStdString()) << endl;
//	ElocDetectDialog::setClient();
	emit readOver();
cout << "socket recieve :" << (str.toStdString()) << endl;
}

#if 0
void ElocDetectDialog::readClient()
{
	time_t lt;
cout << "socket recieve :" << endl;
	QString str = clientConnection->readAll();
	QString str_router = str.mid(18, 17);
	QString str_client = str.mid(1, 17);
	if (str_router == room1_str)
		if (room1_client_str.indexOf(str_client) < 0)
			room1_client_str += "\n" + str_client;
	if (str_router == room2_str)
		if (room2_client_str.indexOf(str_client) < 0)
			room2_client_str += "\n" + str_client;
	if (str_router == room3_str)
		if (room3_client_str.indexOf(str_client) < 0)
			room3_client_str += "\n" + str_client;
	if (str_router == room4_str)
		if (room4_client_str.indexOf(str_client) < 0)
			room4_client_str += "\n" + str_client;
cout << "socket recieve :" << (str.toStdString()) << endl;
//	firstRouterEdit->setText(str.mid(1, 17) + "\n" + str.mid(18, 17));
#if 0
	lt = time(NULL);
	if (lt - start_time >= 2) {
		start_time = lt;
		room1_client_str = room2_client_str = room3_client_str = room4_client_str = "";
	} else {
		firstClientEdit->setText(room1_client_str);
		secondClientEdit->setText(room2_client_str);
		thirdClientEdit->setText(room3_client_str);
		fourthClientEdit->setText(room4_client_str);

		update();
	}
#endif
	setClient();
cout << "socket recieve :" << (str.toStdString()) << endl;
}
#endif

/* added by LTC */
void AutoDetectDialog::autoDetect()
{
#if 1
	QString startTimeStr, endTimeStr;
	string tempStr;
	string numbers("0123456789");
	string::size_type pos;
//	string firstPic, lastPic, fileName, fileNameShort;
	string firstPic, lastPic, fileName;
	int time_legal = 1;

	QDir dir;
	dir.setPath("/home/daqiuqu/work/camera_test");
	dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
	dir.setSorting(QDir::Size | QDir::Reversed);
	QFileInfoList list = dir.entryInfoList();

printf("push button auto detect\n");
	startTimeStr = startTimeEdit->text();
	tempStr = startTimeStr.toStdString();
	firstPic = tempStr;
	cout << tempStr << endl;
	pos = tempStr.find_first_not_of(numbers);
	if (pos != string::npos) {
		printf("start time must be numbers, pos is %ld\n", pos);
		time_legal = 0;
	}
	
	endTimeStr = endTimeEdit->text();
	tempStr = endTimeStr.toStdString();
	lastPic = tempStr;
	cout << tempStr << endl;
	pos = tempStr.find_first_not_of(numbers);
	if (pos != string::npos) {
		printf("end time must be numbers, pos is %ld\n", pos);
		time_legal = 0;
	}

	if (time_legal) {
		for (int i = 0; i < list.size(); ++i) {
			QFileInfo fileInfo = list.at(i);
			fileName = fileInfo.fileName().toStdString();
printf("LTC printf before judge fileName i = %d\n", i);
cout << "LTC print fileName is : " << fileName << endl;
			if (fileName.size() > 14) {
				string fileNameShort(fileName, 0, firstPic.size());
cout << "LTC print fileNameShort is : " << fileNameShort << endl;
				if (fileName >= firstPic && fileName <= lastPic) {
					printf("LTC print before imageDetect\n");
					imageDetect(fileName);
				}
			}
		}
	}
#endif
}

#if 1 
/* Added by LTC */
void AutoDetectDialog::imageDetect(string fileName)
{
//printf("LTC print enter %s\n", __func__);
	Mat img;
	FILE* f = 0;
	char _filename[1024];
	float a[30], b[30];

	string dir("/home/daqiuqu/work/camera_test/");
	string fileNameFull = dir + fileName;

	img = imread(fileNameFull);
//cout << "fileNameFull is " << fileNameFull << endl;
        strcpy(_filename, fileNameFull.c_str());

	HOGDescriptor hog;
	hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
	namedWindow("people detector", 1);

	for(;;)
	{
		vLocEdit->setText(tr(""));
		char* filename = _filename;
		printf("LTC print filename %s: in %s\n", filename, __func__);
		if(!img.data)
		    continue;
		
		fflush(stdout);
		vector<Rect> found, found_filtered;
		double t = (double)getTickCount();
		// run the detector with default parameters. to get a higher hit-rate
		// (and more false alarms, respectively), decrease the hitThreshold and
		// groupThreshold (set groupThreshold to 0 to turn off the grouping completely).
		hog.detectMultiScale(img, found, 0, Size(8,8), Size(32,32), 1.05, 2);
		t = (double)getTickCount() - t;
		printf("tdetection time = %gms\n", t*1000./cv::getTickFrequency());
		size_t i, j;
		for( i = 0; i < found.size(); i++ )
		{
		    Rect r = found[i];
		    for( j = 0; j < found.size(); j++ )
		        if( j != i && (r & found[j]) == r)
		            break;
		    if( j == found.size() )
		        found_filtered.push_back(r);
		}
		for( i = 0; i < found_filtered.size(); i++ )
		{
		    Rect r = found_filtered[i];
		    // the HOG detector returns slightly larger rectangles than the real objects.
		    // so we slightly shrink the rectangles to get a nicer output.
		    r.x += cvRound(r.width*0.1);
		    r.width = cvRound(r.width*0.8);
		    r.y += cvRound(r.height*0.07);
		    r.height = cvRound(r.height*0.8);
		    rectangle(img, r.tl(), r.br(), cv::Scalar(0,255,0), 3);

			// Added for V Loc
			a[i] = r.x + r.width/2;
			b[i] = r.y + r.height;
printf("LTC print before coordinate transformation\n");
			coordinate_transformation(a[i],b[i]);
			peoNum++;
		}
		if (i) {
			updateFlag = 1;
			update();
		} else
			updateFlag = 0;
//		imwrite("/home/daqiuqu/peoDtt.jpg", img);
		imshow("people detector", img);
//		int c = waitKey(0) & 255;
		int c = waitKey(1000) & 255;
		if (c == 'p')
			c = waitKey(0) & 255;
		if (c == 'g')
			return;
		if( c == 'q' || c == 'Q' || !f) {
			printf("LTC print QUIT\n");
			return;
		}
//		    break;
//		if(!f)
//			return;
//		else {
//	    		fclose(f);
//			return;
//		}
	}	
//	if(f)
//	    fclose(f);
}
#endif

/* Added by LTC */
char AutoDetectDialog::coordinate_transformation(float x,float y)
{	
printf("LTC print :Entering %s\n", __func__);
	CvMat* object_points;
	CvMat* image_points;
	CvMat* image_points2;	
	CvMat* homography;
	CvMat* homographyT;
	CvMat* test_points;
	CvMat* result_points;
	
	object_points=cvCreateMat(9,3,CV_32F); 
	image_points=cvCreateMat(9,3,CV_32F);
	image_points2=cvCreateMat(9,3,CV_32F); 
	homography=cvCreateMat(3,3,CV_32F);
	homographyT=cvCreateMat(3,3,CV_32F);
	test_points=cvCreateMat(1,3,CV_32F);
	result_points=cvCreateMat(1,3,CV_32F);
	
	//9¸ö±ê¶¨µãµÄÊµ¼Ê×ø±ê
	CV_MAT_ELEM(*object_points,float,0,0)=0.6;	CV_MAT_ELEM(*object_points,float,0,1)=3.0;   CV_MAT_ELEM(*object_points,float,0,2)=1;
	CV_MAT_ELEM(*object_points,float,1,0)=0.6;	CV_MAT_ELEM(*object_points,float,1,1)=3.6;   CV_MAT_ELEM(*object_points,float,1,2)=1;
	CV_MAT_ELEM(*object_points,float,2,0)=1.2;	CV_MAT_ELEM(*object_points,float,2,1)=2.4;   CV_MAT_ELEM(*object_points,float,2,2)=1;
	CV_MAT_ELEM(*object_points,float,3,0)=1.2;	CV_MAT_ELEM(*object_points,float,3,1)=3.0;   CV_MAT_ELEM(*object_points,float,3,2)=1;
	CV_MAT_ELEM(*object_points,float,4,0)=1.8;	CV_MAT_ELEM(*object_points,float,4,1)=1.8;   CV_MAT_ELEM(*object_points,float,4,2)=1;
	CV_MAT_ELEM(*object_points,float,5,0)=1.8;	CV_MAT_ELEM(*object_points,float,5,1)=2.4;   CV_MAT_ELEM(*object_points,float,5,2)=1;
	CV_MAT_ELEM(*object_points,float,6,0)=1.8;	CV_MAT_ELEM(*object_points,float,6,1)=3.0;   CV_MAT_ELEM(*object_points,float,6,2)=1;
	CV_MAT_ELEM(*object_points,float,7,0)=2.4;	CV_MAT_ELEM(*object_points,float,7,1)=1.2;   CV_MAT_ELEM(*object_points,float,7,2)=1;
	CV_MAT_ELEM(*object_points,float,8,0)=2.4;	CV_MAT_ELEM(*object_points,float,8,1)=3.0;   CV_MAT_ELEM(*object_points,float,8,2)=1;

	//9¸ö±ê¶¨µãÔÚÍ¼ÏñÖÐµÄ×ø±ê
	CV_MAT_ELEM(*image_points,float,0,0)=120;	CV_MAT_ELEM(*image_points,float,0,1)=416;   CV_MAT_ELEM(*image_points,float,0,2)=1;
	CV_MAT_ELEM(*image_points,float,1,0)=80;	CV_MAT_ELEM(*image_points,float,1,1)=474;   CV_MAT_ELEM(*image_points,float,1,2)=1;
	CV_MAT_ELEM(*image_points,float,2,0)=255;	CV_MAT_ELEM(*image_points,float,2,1)=377;   CV_MAT_ELEM(*image_points,float,2,2)=1; 
	CV_MAT_ELEM(*image_points,float,3,0)=243;	CV_MAT_ELEM(*image_points,float,3,1)=424;   CV_MAT_ELEM(*image_points,float,3,2)=1;
	CV_MAT_ELEM(*image_points,float,4,0)=360;	CV_MAT_ELEM(*image_points,float,4,1)=345;   CV_MAT_ELEM(*image_points,float,4,2)=1;
	CV_MAT_ELEM(*image_points,float,5,0)=366;	CV_MAT_ELEM(*image_points,float,5,1)=378;   CV_MAT_ELEM(*image_points,float,5,2)=1;
	CV_MAT_ELEM(*image_points,float,6,0)=375;	CV_MAT_ELEM(*image_points,float,6,1)=424;   CV_MAT_ELEM(*image_points,float,6,2)=1;
	CV_MAT_ELEM(*image_points,float,7,0)=435;	CV_MAT_ELEM(*image_points,float,7,1)=318;   CV_MAT_ELEM(*image_points,float,7,2)=1;
	CV_MAT_ELEM(*image_points,float,8,0)=499;	CV_MAT_ELEM(*image_points,float,8,1)=418;   CV_MAT_ELEM(*image_points,float,8,2)=1;

	//Çóµ¥Ó¦¾ØÕó
	cvFindHomography(image_points,object_points, homography );

	CV_MAT_ELEM(*test_points,float,0,0) = x;
	CV_MAT_ELEM(*test_points,float,0,1) = y;
	CV_MAT_ELEM(*test_points,float,0,2)=1;

	//¼ÆËã×ø±ê
	cvTranspose( homography, homographyT );
	cvMatMul(test_points,homographyT,result_points);
	float x0 = CV_MAT_ELEM(*result_points,float,0,0)/CV_MAT_ELEM(*result_points,float,0,2);
	float y0 = CV_MAT_ELEM(*result_points,float,0,1)/CV_MAT_ELEM(*result_points,float,0,2);

	xPos[peoNum] = x0;
	yPos[peoNum] = y0;
printf("Pos debug peoNum = %d, xPos is %f, yPos is %f\n", peoNum, xPos[peoNum], yPos[peoNum]);
	char vLoc[100];
	sprintf(vLoc, "%s(%6.2f,%6.2f);", qPrintable(vLocEdit->text()), x0, y0);
	vLocEdit->setText(vLoc);
	
	return *vLoc;
}

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("Preferences"));

	int height = parent->size().height() - 50;
	if (height > 800)
		height = 800;
	resize(600, height);

	QWidget *optsWidgetArea = new QWidget(this);
	QScrollArea *scrollArea = new QScrollArea;
	scrollArea->setWidget(optsWidgetArea);
	scrollArea->setWidgetResizable(true);
	scrollArea->setFrameShadow(QFrame::Plain);
	QHBoxLayout *buttonsHbox = new QHBoxLayout;
    QPushButton *okButton = new QPushButton(tr("OK"));
    okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(okButton, SIGNAL(clicked()), this, SLOT(saveSettings()));
	QPushButton *closeButton = new QPushButton(tr("Cancel"));
	closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(closeButton, SIGNAL(clicked()), this, SLOT(abort()));
	buttonsHbox->addWidget(closeButton, 1, Qt::AlignRight);
	buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);

	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addWidget(scrollArea);
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);

	// imageView background color
	QLabel *backgroundColorLab = new QLabel(tr("Background color: "));
	backgroundColorButton = new QToolButton();
	backgroundColorButton->setFixedSize(48, 24);
	QHBoxLayout *bgColBox = new QHBoxLayout;
	bgColBox->addWidget(backgroundColorLab);
	bgColBox->addWidget(backgroundColorButton);
	bgColBox->addStretch(1);
	connect(backgroundColorButton, SIGNAL(clicked()), this, SLOT(pickColor()));
	setButtonBgColor(GData::backgroundColor, backgroundColorButton);
	backgroundColorButton->setAutoFillBackground(true);
	bgColor = GData::backgroundColor;

	// thumbView background color
	QLabel *bgThumbTxtLab = new QLabel(tr("Background color: "));
	colThumbButton = new QToolButton();
	colThumbButton->setFixedSize(48, 24);
	QHBoxLayout *bgThumbColBox = new QHBoxLayout;
	bgThumbColBox->addWidget(bgThumbTxtLab);
	bgThumbColBox->addWidget(colThumbButton);
	connect(colThumbButton, SIGNAL(clicked()), this, SLOT(pickThumbsColor()));
	setButtonBgColor(GData::thumbsBackgroundColor, colThumbButton);
	colThumbButton->setAutoFillBackground(true);
	thumbBgColor = GData::thumbsBackgroundColor;

	// thumbView text color
	QLabel *txtThumbTxtLab = new QLabel(tr("\tLabel color: "));
	colThumbTextButton = new QToolButton();
	colThumbTextButton->setFixedSize(48, 24);
	bgThumbColBox->addWidget(txtThumbTxtLab);
	bgThumbColBox->addWidget(colThumbTextButton);
	bgThumbColBox->addStretch(1);
	connect(colThumbTextButton, SIGNAL(clicked()), this, SLOT(pickThumbsTextColor()));
	setButtonBgColor(GData::thumbsTextColor, colThumbTextButton);
	colThumbTextButton->setAutoFillBackground(true);
	thumbTextColor = GData::thumbsTextColor;

	// Thumbnail spacing
	QLabel *thumbSpacingLab = new QLabel(tr("Add space between thumbnails: "));
	thumbSpacingSpin = new QSpinBox;
	thumbSpacingSpin->setRange(0, 15);
	thumbSpacingSpin->setValue(GData::thumbSpacing);
	QHBoxLayout *thumbSpacingHbox = new QHBoxLayout;
	thumbSpacingHbox->addWidget(thumbSpacingLab);
	thumbSpacingHbox->addWidget(thumbSpacingSpin);
	thumbSpacingHbox->addStretch(1);

	// Do not enlarge small thumbs
	noSmallThumbCb = new 
				QCheckBox(tr("Show original size of images smaller than the thumbnail size"), this);
	noSmallThumbCb->setChecked(GData::noEnlargeSmallThumb);

	// Thumbnail pages to read ahead
	QLabel *thumbPagesLab = new QLabel(tr("Number of thumbnail pages to read ahead: "));
	thumbPagesSpin = new QSpinBox;
	thumbPagesSpin->setRange(1, 10);
	thumbPagesSpin->setValue(GData::thumbPagesReadahead);
	QHBoxLayout *thumbPagesHbox = new QHBoxLayout;
	thumbPagesHbox->addWidget(thumbPagesLab);
	thumbPagesHbox->addWidget(thumbPagesSpin);
	thumbPagesHbox->addStretch(1);

	// Thumbnail options
	QGroupBox *thumbOptsGroupBox = new QGroupBox(tr("Thumbnails"));
	QVBoxLayout *thumbsOptsBox = new QVBoxLayout;
	thumbsOptsBox->addLayout(thumbSpacingHbox);
	thumbsOptsBox->addWidget(noSmallThumbCb);
	thumbsOptsBox->addLayout(bgThumbColBox);
	thumbsOptsBox->addLayout(thumbPagesHbox);
	thumbOptsGroupBox->setLayout(thumbsOptsBox);

	// Zoom large images
	QGroupBox *fitLargeGroupBox = new QGroupBox(tr("Fit Large Images"));
	fitLargeRadios[0] = new QRadioButton(tr("Disable"));
	fitLargeRadios[1] = new QRadioButton(tr("By width and height"));
	fitLargeRadios[2] = new QRadioButton(tr("By width"));
	fitLargeRadios[3] = new QRadioButton(tr("By height"));
	fitLargeRadios[4] = new QRadioButton(tr("Stretch disproportionately"));
	QVBoxLayout *fitLargeVbox = new QVBoxLayout;
	for (int i = 0; i < nZoomRadios; ++i)
	{
		fitLargeVbox->addWidget(fitLargeRadios[i]);
		fitLargeRadios[i]->setChecked(false);
	}
	fitLargeVbox->addStretch(1);
	fitLargeGroupBox->setLayout(fitLargeVbox);
	fitLargeRadios[GData::zoomOutFlags]->setChecked(true);
 	
	// Zoom small images
	QGroupBox *fitSmallGroupBox = new QGroupBox(tr("Fit Small Images"));
	fitSmallRadios[0] = new QRadioButton(tr("Disable"));
	fitSmallRadios[1] = new QRadioButton(tr("By width and height"));
	fitSmallRadios[2] = new QRadioButton(tr("By width"));
	fitSmallRadios[3] = new QRadioButton(tr("By height"));
	fitSmallRadios[4] = new QRadioButton(tr("Stretch disproportionately"));
	QVBoxLayout *fitSmallVbox = new QVBoxLayout;
	for (int i = 0; i < nZoomRadios; ++i)
	{
		fitSmallVbox->addWidget(fitSmallRadios[i]);
		fitSmallRadios[i]->setChecked(false);
	}
	fitSmallVbox->addStretch(1);
	fitSmallGroupBox->setLayout(fitSmallVbox);
	fitSmallRadios[GData::zoomInFlags]->setChecked(true);

	// Exit when opening image
	exitCliCb = new 
				QCheckBox(tr("Exit instead of closing, when image is loaded from command line"), this);
	exitCliCb->setChecked(GData::exitInsteadOfClose);

	// Exit when opening image
	wrapListCb = new QCheckBox(tr("Wrap image list when reaching last or first image"), this);
	wrapListCb->setChecked(GData::wrapImageList);

	// Save quality
	QLabel *saveQualityLab = new QLabel(tr("Default quality when saving images: "));
	saveQualitySpin = new QSpinBox;
	saveQualitySpin->setRange(0, 100);
	saveQualitySpin->setValue(GData::defaultSaveQuality);
	QHBoxLayout *saveQualityHbox = new QHBoxLayout;
	saveQualityHbox->addWidget(saveQualityLab);
	saveQualityHbox->addWidget(saveQualitySpin);
	saveQualityHbox->addStretch(1);

	// Enable animations
	enableAnimCb = new QCheckBox(tr("Enable GIF animation"), this);
	enableAnimCb->setChecked(GData::enableAnimations);

	// Enable Exif
	enableExifCb = new QCheckBox(tr("Rotate according to Exif orientation"), this);
	enableExifCb->setChecked(GData::exifRotationEnabled);

	// Startup directory
	QGroupBox *startupDirGroupBox = new QGroupBox(tr("Startup folder"));
	startupDirRadios[GData::defaultDir] = new QRadioButton(tr("Default, or specified by command line argument"));
	startupDirRadios[GData::rememberLastDir] = new QRadioButton(tr("Remember last"));
	startupDirRadios[GData::specifiedDir] = new QRadioButton(tr("Specify:"));
	
	startupDirEdit = new QLineEdit;
	startupDirEdit->setClearButtonEnabled(true);
	startupDirEdit->setMinimumWidth(300);
	startupDirEdit->setMaximumWidth(400);

	QToolButton *chooseStartupDirButton = new QToolButton();
	chooseStartupDirButton->setIcon(QIcon::fromTheme("document-open", QIcon(":/images/open.png")));
	chooseStartupDirButton->setFixedSize(26, 26);
	chooseStartupDirButton->setIconSize(QSize(16, 16));
	connect(chooseStartupDirButton, SIGNAL(clicked()), this, SLOT(pickStartupDir()));
	
	QHBoxLayout *startupDirEditBox = new QHBoxLayout;
	startupDirEditBox->addWidget(startupDirRadios[2]);
	startupDirEditBox->addWidget(startupDirEdit);
	startupDirEditBox->addWidget(chooseStartupDirButton);
	startupDirEditBox->addStretch(1);

	QVBoxLayout *startupDirVbox = new QVBoxLayout;
	for (int i = 0; i < 2; ++i)
	{
		startupDirVbox->addWidget(startupDirRadios[i]);
		startupDirRadios[i]->setChecked(false);
	}
	startupDirVbox->addLayout(startupDirEditBox);
	startupDirVbox->addStretch(1);
	startupDirGroupBox->setLayout(startupDirVbox);

	if (GData::startupDir == GData::specifiedDir)
		startupDirRadios[GData::specifiedDir]->setChecked(true);
	else if (GData::startupDir == GData::rememberLastDir)
		startupDirRadios[GData::rememberLastDir]->setChecked(true);
	else
		startupDirRadios[GData::defaultDir]->setChecked(true);
	startupDirEdit->setText(GData::specifiedStartDir);

	// Viewer options
	QVBoxLayout *viewerOptsBox = new QVBoxLayout;
	QHBoxLayout *zoomOptsBox = new QHBoxLayout;
	zoomOptsBox->setAlignment(Qt::AlignTop);
	zoomOptsBox->addWidget(fitLargeGroupBox);
	zoomOptsBox->addWidget(fitSmallGroupBox);
	zoomOptsBox->addStretch(1);
	viewerOptsBox->addLayout(zoomOptsBox);
	viewerOptsBox->addLayout(bgColBox);
	viewerOptsBox->addWidget(wrapListCb);
	viewerOptsBox->addLayout(saveQualityHbox);
	viewerOptsBox->addWidget(enableAnimCb);
	viewerOptsBox->addWidget(enableExifCb);
	viewerOptsBox->addWidget(startupDirGroupBox);
	viewerOptsBox->addWidget(exitCliCb);
	QGroupBox *viewerOptsGrp = new QGroupBox(tr("Viewer"));
	viewerOptsGrp->setLayout(viewerOptsBox);

	// Slide show delay
	QLabel *slideDelayLab = new QLabel(tr("Delay between slides in seconds: "));
	slideDelaySpin = new QSpinBox;
	slideDelaySpin->setRange(1, 3600);
	slideDelaySpin->setValue(GData::slideShowDelay);
	QHBoxLayout *slideDelayHbox = new QHBoxLayout;
	slideDelayHbox->addWidget(slideDelayLab);
	slideDelayHbox->addWidget(slideDelaySpin);
	slideDelayHbox->addStretch(1);

	// Slide show random
	slideRandomCb = new QCheckBox(tr("Show random images"), this);
	slideRandomCb->setChecked(GData::slideShowRandom);

	// Slide show options
	QVBoxLayout *slideShowVbox = new QVBoxLayout;
	slideShowVbox->addLayout(slideDelayHbox);
	slideShowVbox->addWidget(slideRandomCb);
	QGroupBox *slideShowGbox = new QGroupBox(tr("Slide Show"));
	slideShowGbox->setLayout(slideShowVbox);

	// Keyboard shortcuts widgets
	QComboBox *keysCombo = new QComboBox();
	keyLine = new KeyGrabLineEdit(this, keysCombo);
	connect(keyLine, SIGNAL(textChanged(const QString&)), keyLine, SLOT(clearShortcut()));
	connect(keysCombo, SIGNAL(activated(const QString &)),
							this, SLOT(setActionKeyText(const QString &)));

	QMapIterator<QString, QAction *> it(GData::actionKeys);
	while (it.hasNext())
	{
		it.next();
		keysCombo->addItem(it.key());
	}
	keyLine->setText(GData::actionKeys.value(keysCombo->currentText())->shortcut().toString());

	// Mouse settings
	reverseMouseCb = new QCheckBox(tr("Swap mouse left-click and middle-click actions"), this);
	reverseMouseCb->setChecked(GData::reverseMouseBehavior);

	// Keyboard and mouse group
	QHBoxLayout *keyboardHbox = new QHBoxLayout;
	keyboardHbox->addWidget(keysCombo);
	keyboardHbox->addWidget(keyLine);
	keyboardHbox->addStretch(1);

	QVBoxLayout *mouseVbox = new QVBoxLayout;
	mouseVbox->addLayout(keyboardHbox);
	mouseVbox->addWidget(reverseMouseCb);

	QGroupBox *keyboardGbox = new QGroupBox(tr("Keyboard and Mouse"));
	keyboardGbox->setLayout(mouseVbox);

	// General
	QVBoxLayout *optsLayout = new QVBoxLayout;
	optsWidgetArea->setLayout(optsLayout);
	optsLayout->addWidget(viewerOptsGrp);
	optsLayout->addSpacerItem(new QSpacerItem(0, 5, QSizePolicy::Fixed, QSizePolicy::Expanding));
	optsLayout->addWidget(thumbOptsGroupBox);
	optsLayout->addWidget(slideShowGbox);
	optsLayout->addWidget(keyboardGbox);
	optsLayout->addStretch(1);
}

void SettingsDialog::saveSettings()
{
	int i;

	for (i = 0; i < nZoomRadios; ++i)
	{
		if (fitLargeRadios[i]->isChecked())
		{
			GData::zoomOutFlags = i;
			GData::appSettings->setValue("zoomOutFlags", (int)GData::zoomOutFlags);
			break;
		}
	}

	for (i = 0; i < nZoomRadios; ++i)
	{
		if (fitSmallRadios[i]->isChecked())
		{
			GData::zoomInFlags = i;
			GData::appSettings->setValue("zoomInFlags", (int)GData::zoomInFlags);
			break;
		}
	}

	GData::backgroundColor = bgColor;
	GData::thumbsBackgroundColor = thumbBgColor;
	GData::thumbsTextColor = thumbTextColor;
	GData::thumbSpacing = thumbSpacingSpin->value();
	GData::thumbPagesReadahead = thumbPagesSpin->value();
	GData::exitInsteadOfClose = exitCliCb->isChecked();
	GData::wrapImageList = wrapListCb->isChecked();
	GData::defaultSaveQuality = saveQualitySpin->value();
	GData::noEnlargeSmallThumb = noSmallThumbCb->isChecked();
	GData::slideShowDelay = slideDelaySpin->value();
	GData::slideShowRandom = slideRandomCb->isChecked();
	GData::enableAnimations = enableAnimCb->isChecked();
	GData::exifRotationEnabled = enableExifCb->isChecked();
	GData::reverseMouseBehavior = reverseMouseCb->isChecked();

	if (startupDirRadios[0]->isChecked())
		GData::startupDir = GData::defaultDir;
	else if (startupDirRadios[1]->isChecked())
		GData::startupDir = GData::rememberLastDir;
	else 
	{
		GData::startupDir = GData::specifiedDir;
		GData::specifiedStartDir = startupDirEdit->text();
	
	}

	accept();
}

void SettingsDialog::abort()
{
	reject();
}

void SettingsDialog::pickColor()
{
	QColor userColor = QColorDialog::getColor(GData::backgroundColor, this);
    if (userColor.isValid())
    {	
		setButtonBgColor(userColor, backgroundColorButton);
        bgColor = userColor;
    }
}

void SettingsDialog::setButtonBgColor(QColor &color, QToolButton *button)
{
	QString style = "background: rgb(%1, %2, %3);";
	style = style.arg(color.red()).arg(color.green()).arg(color.blue());
	button->setStyleSheet(style);
}

void SettingsDialog::pickThumbsColor()
{
	QColor userColor = QColorDialog::getColor(GData::thumbsBackgroundColor, this);
	if (userColor.isValid())
	{	
		setButtonBgColor(userColor, colThumbButton);
		thumbBgColor = userColor;
	}
}

void SettingsDialog::pickThumbsTextColor()
{
	QColor userColor = QColorDialog::getColor(GData::thumbsTextColor, this);
	if (userColor.isValid())
	{	
		setButtonBgColor(userColor, colThumbTextButton);
		thumbTextColor = userColor;
	}
}

void SettingsDialog::pickStartupDir()
{
	QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose Startup Folder"), "",
									QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	startupDirEdit->setText(dirName);
}

CropDialog::CropDialog(QWidget *parent, ImageView *imageView_) : QDialog(parent)
{
	setWindowTitle(tr("Cropping"));
	resize(400, 400);
	if (GData::dialogLastX)
		move(GData::dialogLastX, GData::dialogLastY);
	imageView = imageView_;

	QHBoxLayout *buttonsHbox = new QHBoxLayout;
	QPushButton *okButton = new QPushButton(tr("OK"));
	okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(okButton, SIGNAL(clicked()), this, SLOT(ok()));
	buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);

	QSlider *topSlide = new QSlider(Qt::Vertical);
	QSlider *bottomSlide = new QSlider(Qt::Vertical);
	QSlider *leftSlide = new QSlider(Qt::Horizontal);
	QSlider *rightSlide = new QSlider(Qt::Horizontal);

	topSlide->setTickPosition(QSlider::TicksAbove);
	topSlide->setInvertedAppearance(true);
	topSlide->setTickInterval(100);
	bottomSlide->setTickPosition(QSlider::TicksBelow);
	bottomSlide->setTickInterval(100);
	leftSlide->setTickPosition(QSlider::TicksAbove);
	leftSlide->setTickInterval(100);
	rightSlide->setTickPosition(QSlider::TicksBelow);
	rightSlide->setInvertedAppearance(true);
	rightSlide->setTickInterval(100);

	topSpin = new QSpinBox;
	bottomSpin = new QSpinBox;
	leftSpin = new QSpinBox;
	rightSpin = new QSpinBox;

	QGridLayout *mainGbox = new QGridLayout;

	QLabel *topLab = new QLabel(tr("Top"));
	QLabel *leftLab = new QLabel(tr("Left"));
	QLabel *rightLab = new QLabel(tr("Right"));
	QLabel *bottomLab = new QLabel(tr("Bottom"));

	QHBoxLayout *topBox = new QHBoxLayout;
	topBox->addWidget(topLab);
	topBox->addWidget(topSpin);
	topBox->addStretch(1);	

	QHBoxLayout *bottomBox = new QHBoxLayout;
	bottomBox->addWidget(bottomLab);
	bottomBox->addWidget(bottomSpin);
	bottomBox->addStretch(1);	

	QHBoxLayout *leftBox = new QHBoxLayout;
	leftBox->addWidget(leftLab);
	leftBox->addWidget(leftSpin);
	leftBox->addStretch(1);	

	QHBoxLayout *rightBox = new QHBoxLayout;
	rightBox->addWidget(rightLab);
	rightBox->addWidget(rightSpin);
	rightBox->addStretch(1);	

	mainGbox->addWidget(topSlide, 2, 1, 5, 1);
	mainGbox->addLayout(topBox, 4, 2, 1, 1);
	mainGbox->addWidget(bottomSlide, 2, 7, 5, 1);
	mainGbox->addLayout(bottomBox, 4, 6, 1, 1);
	mainGbox->addWidget(leftSlide, 1, 2, 1, 5);
	mainGbox->addLayout(leftBox, 2, 4, 1, 1);
	mainGbox->addWidget(rightSlide, 7, 2, 1, 5);
	mainGbox->addLayout(rightBox, 6, 4, 1, 1);

	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addLayout(mainGbox);
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);

	int width = imageView->getImageWidthPreCropped();
	int height = imageView->getImageHeightPreCropped();

	topSpin->setRange(0, height);
	bottomSpin->setRange(0, height);
	leftSpin->setRange(0, width);
	rightSpin->setRange(0, width);
	topSlide->setRange(0, height);
	bottomSlide->setRange(0, height);
	leftSlide->setRange(0, width);
	rightSlide->setRange(0, width);

	connect(topSlide, SIGNAL(valueChanged(int)), topSpin, SLOT(setValue(int)));
	connect(bottomSlide, SIGNAL(valueChanged(int)), bottomSpin, SLOT(setValue(int)));
	connect(leftSlide, SIGNAL(valueChanged(int)), leftSpin, SLOT(setValue(int)));
	connect(rightSlide, SIGNAL(valueChanged(int)), rightSpin, SLOT(setValue(int)));
	connect(topSpin, SIGNAL(valueChanged(int)), topSlide, SLOT(setValue(int)));
	connect(bottomSpin, SIGNAL(valueChanged(int)), bottomSlide, SLOT(setValue(int)));
	connect(leftSpin, SIGNAL(valueChanged(int)), leftSlide, SLOT(setValue(int)));
	connect(rightSpin, SIGNAL(valueChanged(int)), rightSlide, SLOT(setValue(int)));

	topSpin->setValue(GData::cropTop);
	bottomSpin->setValue(GData::cropHeight);
	leftSpin->setValue(GData::cropLeft);
	rightSpin->setValue(GData::cropWidth);

	connect(topSpin, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
	connect(bottomSpin, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
	connect(leftSpin, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
	connect(rightSpin, SIGNAL(valueChanged(int)), this, SLOT(applyCrop(int)));
}

void CropDialog::applyCrop(int)
{
	GData::cropLeft = leftSpin->value();
	GData::cropTop = topSpin->value();
	GData::cropWidth = rightSpin->value();
	GData::cropHeight = bottomSpin->value();
	imageView->refresh();
}

void CropDialog::ok()
{
	GData::dialogLastX = pos().x();
	GData::dialogLastY = pos().y(); 
	accept();
}

ColorsDialog::ColorsDialog(QWidget *parent, ImageView *imageView_) : QDialog(parent)
{
	setWindowTitle(tr("Colors"));
	resize(500, 200);
	if (GData::dialogLastX)
		move(GData::dialogLastX, GData::dialogLastY);
	imageView = imageView_;

	QHBoxLayout *buttonsHbox = new QHBoxLayout;
	QPushButton *resetButton = new QPushButton(tr("Reset"));
	resetButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(resetButton, SIGNAL(clicked()), this, SLOT(reset()));
	buttonsHbox->addWidget(resetButton, 0, Qt::AlignLeft);
	QPushButton *okButton = new QPushButton(tr("OK"));
	okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(okButton, SIGNAL(clicked()), this, SLOT(ok()));
	buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);

	QLabel *hueLab = new QLabel(tr("Hue"));
	QLabel *satLab = new QLabel(tr("Saturation"));
	QLabel *lightLab = new QLabel(tr("Lightness"));
	QLabel *channelsLab = new QLabel(tr("Channels"));

	hueSatEnabledCb = new QCheckBox(tr("Enable"), this);
	hueSatEnabledCb->setCheckState(GData::hueSatEnabled? Qt::Checked : Qt::Unchecked);
	connect(hueSatEnabledCb, SIGNAL(stateChanged(int)), this, SLOT(enableHueSat(int)));	

	hueSlide = new QSlider(Qt::Horizontal);
	hueSlide->setTickPosition(QSlider::TicksAbove);
	hueSlide->setTickInterval(25);
	hueSlide->setRange(0, 255);
	hueSlide->setTracking(false);
	hueSlide->setValue(GData::hueVal);
	connect(hueSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	colorizeCb = new QCheckBox(tr("Colorize"), this);
	colorizeCb->setCheckState(GData::colorizeEnabled? Qt::Checked : Qt::Unchecked);
	connect(colorizeCb, SIGNAL(stateChanged(int)), this, SLOT(enableColorize(int)));	

	saturationSlide = new QSlider(Qt::Horizontal);
	saturationSlide->setTickPosition(QSlider::TicksAbove);
	saturationSlide->setTickInterval(25);
	saturationSlide->setRange(0, 500);
	saturationSlide->setTracking(false);
	saturationSlide->setValue(GData::saturationVal);
	connect(saturationSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	lightnessSlide = new QSlider(Qt::Horizontal);
	lightnessSlide->setTickPosition(QSlider::TicksAbove);
	lightnessSlide->setTickInterval(25);
	lightnessSlide->setRange(0, 500);
	lightnessSlide->setTracking(false);
	lightnessSlide->setValue(GData::lightnessVal);
	connect(lightnessSlide, SIGNAL(valueChanged(int)), this, SLOT(applyColors(int)));

	QHBoxLayout *channelsHbox = new QHBoxLayout;
	redB = new QCheckBox(tr("Red"));
	redB->setCheckable(true);
	redB->setChecked(GData::hueRedChannel);
	connect(redB, SIGNAL(clicked()), this, SLOT(setRedChannel()));
	channelsHbox->addWidget(redB, 0, Qt::AlignLeft);
	greenB = new QCheckBox(tr("Green"));
	greenB->setCheckable(true);
	greenB->setChecked(GData::hueGreenChannel);
	connect(greenB, SIGNAL(clicked()), this, SLOT(setGreenChannel()));
	channelsHbox->addWidget(greenB, 0, Qt::AlignLeft);
	blueB = new QCheckBox(tr("Blue"));
	blueB->setCheckable(true);
	blueB->setChecked(GData::hueBlueChannel);
	connect(blueB, SIGNAL(clicked()), this, SLOT(setBlueChannel()));
	channelsHbox->addWidget(blueB, 0, Qt::AlignLeft);
	channelsHbox->addStretch(1);

	QGridLayout *colChannelbox = new QGridLayout;
	colChannelbox->addWidget(hueSatEnabledCb,	0, 0, Qt::AlignLeft);
	colChannelbox->addWidget(hueLab,				1, 0, 1, 1);
	colChannelbox->addWidget(hueSlide,			1, 1, 1, 1);
	colChannelbox->addWidget(colorizeCb,			2, 1, 1, 1);
	colChannelbox->addWidget(satLab,				3, 0, 1, 1);
	colChannelbox->addWidget(saturationSlide,	3, 1, 1, 1);
	colChannelbox->addWidget(lightLab, 			4, 0, 1, 1);
	colChannelbox->addWidget(lightnessSlide,		4, 1, 1, 1);
	colChannelbox->addWidget(channelsLab,		5, 0, 1, 1);
	colChannelbox->addLayout(channelsHbox,		5, 1, 1, 1);
	// colChannelbox->setColumnStretch(3, 1);

	QGroupBox *colChannelgrp = new QGroupBox(tr("Hue and Saturation"));
	colChannelgrp->setLayout(colChannelbox);
	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addWidget(colChannelgrp);
	mainVbox->addStretch(1);	
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);
}

void ColorsDialog::applyColors(int)
{
	GData::hueVal = hueSlide->value();
	GData::saturationVal = saturationSlide->value();
	GData::lightnessVal = lightnessSlide->value();

	imageView->refresh();
}

void ColorsDialog::ok()
{
	GData::dialogLastX = pos().x();
	GData::dialogLastY = pos().y(); 
	accept();
}

void ColorsDialog::reset()
{
	hueSlide->setValue(0);
	colorizeCb->setChecked(false);
	saturationSlide->setValue(100);
	lightnessSlide->setValue(100);
	redB->setChecked(true);
	greenB->setChecked(true);
	blueB->setChecked(true);
	GData::hueRedChannel = true;
	GData::hueGreenChannel = true;
	GData::hueBlueChannel = true;
	imageView->refresh();
}

void ColorsDialog::enableHueSat(int state)
{
	GData::hueSatEnabled = state;
	imageView->refresh();
}

void ColorsDialog::enableColorize(int state)
{
	GData::colorizeEnabled = state;
	imageView->refresh();
}

void ColorsDialog::setRedChannel()
{
	GData::hueRedChannel = redB->isChecked();
	imageView->refresh();
}

void ColorsDialog::setGreenChannel()
{
	GData::hueGreenChannel = greenB->isChecked();
	imageView->refresh();
}

void ColorsDialog::setBlueChannel()
{
	GData::hueBlueChannel = blueB->isChecked();
	imageView->refresh();
}

void AppMgmtDialog::addTableModelItem(QStandardItemModel *model, QString &key, QString &val)
{
	int atRow = model->rowCount();
	QStandardItem *itemKey = new QStandardItem(key);
	QStandardItem *itemKey2 = new QStandardItem(val);
	model->insertRow(atRow, itemKey);
	model->setItem(atRow, 1, itemKey2);
}

AppMgmtDialog::AppMgmtDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("Manage External Applications"));
	resize(350, 250);

	appsTable = new QTableView(this);
	appsTable->setSelectionBehavior(QAbstractItemView::SelectItems);
	appsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
	appsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	appsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	appsTableModel = new QStandardItemModel(this);
	appsTable->setModel(appsTableModel);
	appsTable->verticalHeader()->setVisible(false);
	appsTable->verticalHeader()->setDefaultSectionSize(appsTable->verticalHeader()->minimumSectionSize());
	appsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	appsTableModel->setHorizontalHeaderItem(0, new QStandardItem(QString(tr("Name"))));
	appsTableModel->setHorizontalHeaderItem(1, new QStandardItem(QString(tr("Path"))));
	appsTable->	setShowGrid(false);

	QHBoxLayout *addRemoveHbox = new QHBoxLayout;
    QPushButton *addButton = new QPushButton(tr("Add"));
	connect(addButton, SIGNAL(clicked()), this, SLOT(add()));
	addRemoveHbox->addWidget(addButton, 0, Qt::AlignRight);
    QPushButton *removeButton = new QPushButton(tr("Remove"));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
	addRemoveHbox->addWidget(removeButton, 0, Qt::AlignRight);
	addRemoveHbox->addStretch(1);	

	QHBoxLayout *buttonsHbox = new QHBoxLayout;
    QPushButton *okButton = new QPushButton(tr("OK"));
    okButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(okButton, SIGNAL(clicked()), this, SLOT(ok()));
	buttonsHbox->addWidget(okButton, 0, Qt::AlignRight);
	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addWidget(appsTable);
	mainVbox->addLayout(addRemoveHbox);
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);

	// Load external apps list
	QString key, val;
	QMapIterator<QString, QString> it(GData::externalApps);
	while (it.hasNext())
	{
		it.next();
		key = it.key();
		val = it.value();
		addTableModelItem(appsTableModel, key, val);
	}
}

void AppMgmtDialog::ok()
{
	int row = appsTableModel->rowCount();
	GData::externalApps.clear();
    for (int i = 0; i < row ; ++i)
    {
		GData::externalApps[appsTableModel->itemFromIndex(appsTableModel->index(i, 0))->text()] =
						appsTableModel->itemFromIndex(appsTableModel->index(i, 1))->text();
   	}

	accept();
}

void AppMgmtDialog::add()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Choose Application"), "", "");
	if (fileName.isEmpty())
		return;
		
	QFileInfo fileInfo = QFileInfo(fileName);
	if (!fileInfo.isExecutable())
	{
		QMessageBox msgBox;
		msgBox.critical(this, tr("Error"), tr("Not an executable"));
		return;
	}
	
	QString appName = fileInfo.fileName();
	addTableModelItem(appsTableModel, appName, fileName);
}

void AppMgmtDialog::remove()
{
	QModelIndexList indexesList;
	while((indexesList = appsTable->selectionModel()->selectedIndexes()).size())
	{
		appsTableModel->removeRow(indexesList.first().row());
	}
}

CopyMoveToDialog::CopyMoveToDialog(QWidget *parent, QString thumbsPath) : QDialog(parent)
{
	setWindowTitle(tr("Copy or Move Images to..."));
	resize(350, 250);
	currentPath = thumbsPath;

	pathsTable = new QTableView(this);
	pathsTable->setSelectionBehavior(QAbstractItemView::SelectItems);
	pathsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
	pathsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	pathsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	pathsTable->setSelectionMode(QAbstractItemView::SingleSelection);
	pathsTableModel = new QStandardItemModel(this);
	pathsTable->setModel(pathsTableModel);
	pathsTable->verticalHeader()->setVisible(false);
	pathsTable->horizontalHeader()->setVisible(false);
	pathsTable->verticalHeader()->setDefaultSectionSize(pathsTable->verticalHeader()->
																				minimumSectionSize());
	pathsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	pathsTable->	setShowGrid(false);

	QHBoxLayout *addRemoveHbox = new QHBoxLayout;
    QPushButton *addButton = new QPushButton(tr("Add"));
	connect(addButton, SIGNAL(clicked()), this, SLOT(add()));
	addRemoveHbox->addWidget(addButton, 0, Qt::AlignRight);
    QPushButton *removeButton = new QPushButton(tr("Remove"));
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));
	addRemoveHbox->addWidget(removeButton, 0, Qt::AlignRight);
	addRemoveHbox->addStretch(1);	

	QHBoxLayout *buttonsHbox = new QHBoxLayout;
    QPushButton *closeButton = new QPushButton(tr("Close"));
    closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(closeButton, SIGNAL(clicked()), this, SLOT(justClose()));
    QPushButton *copyButton = new QPushButton(tr("Copy"));
    copyButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(copyButton, SIGNAL(clicked()), this, SLOT(copy()));
    QPushButton *moveButton = new QPushButton(tr("Move"));
    moveButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	connect(moveButton, SIGNAL(clicked()), this, SLOT(move()));
	buttonsHbox->addStretch(1);	
	buttonsHbox->addWidget(closeButton, 0, Qt::AlignRight);
	buttonsHbox->addWidget(copyButton, 0, Qt::AlignRight);
	buttonsHbox->addWidget(moveButton, 0, Qt::AlignRight);
	
	QVBoxLayout *mainVbox = new QVBoxLayout;
	mainVbox->addWidget(pathsTable);
	mainVbox->addLayout(addRemoveHbox);
	mainVbox->addLayout(buttonsHbox);
	setLayout(mainVbox);

	// Load paths list
	QString key, val;
	QSetIterator<QString> it(GData::copyMoveToPaths);
	while (it.hasNext())
	{
		QStandardItem *item = new QStandardItem(it.next());
		pathsTableModel->insertRow(pathsTableModel->rowCount(), item);
	}
}

void CopyMoveToDialog::savePaths()
{
	GData::copyMoveToPaths.clear();
    for (int i = 0; i < pathsTableModel->rowCount(); ++i)
    {
    	GData::copyMoveToPaths.insert
    						(pathsTableModel->itemFromIndex(pathsTableModel->index(i, 0))->text());
   	}
}

void CopyMoveToDialog::copyOrMove(bool copy)
{
	savePaths();
	copyOp = copy;

	QModelIndexList indexesList;
	if((indexesList = pathsTable->selectionModel()->selectedIndexes()).size())
	{
		selectedPath = pathsTableModel->itemFromIndex(indexesList.first())->text();
		accept();
	}
	else
		reject();
}

void CopyMoveToDialog::copy()
{
	copyOrMove(true);
}

void CopyMoveToDialog::move()
{
	copyOrMove(false);
}

void CopyMoveToDialog::justClose()
{
	savePaths();
	reject();
}

void CopyMoveToDialog::add()
{
	QString dirName = QFileDialog::getExistingDirectory(this, tr("Choose Folder"), currentPath,
									QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (dirName.isEmpty())
		return;
		
	QStandardItem *item = new QStandardItem(dirName);
	pathsTableModel->insertRow(pathsTableModel->rowCount(), item);

	pathsTable->selectionModel()->clearSelection();
	pathsTable->selectionModel()->select(pathsTableModel->index(pathsTableModel->rowCount() - 1, 0),
																			QItemSelectionModel::Select);
}

void CopyMoveToDialog::remove()
{
	QModelIndexList indexesList;
	if((indexesList = pathsTable->selectionModel()->selectedIndexes()).size())
	{
		pathsTableModel->removeRow(indexesList.first().row());
	}
}

