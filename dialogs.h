/*
 *  Copyright (C) 2013 Ofer Kashayov <oferkv@live.com>
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

#ifndef DIALOGS_H
#define DIALOGS_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <QtWidgets>
#include <QComboBox>
#include <QtNetwork>
#include "thumbview.h"
#include "imageview.h"

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace std;

int cpMvFile(bool isCopy, QString &srcFile, QString &srcPath, QString &dstPath, QString &dstDir);

class CpMvDialog : public QDialog
{
    Q_OBJECT

public slots:
	void abort();

public:
    CpMvDialog(QWidget *parent);
	void exec(ThumbView *thumbView, QString &destDir, bool pasteInCurrDir);
	int nfiles;

private:
	QLabel *opLabel;
	QPushButton *cancelButton;
	QFileInfo *dirInfo;
	bool abortOp;
};

class KeyGrabLineEdit : public QLineEdit
{
	Q_OBJECT
 
public:
	KeyGrabLineEdit(QWidget *parent, QComboBox *combo);

public slots:
	void clearShortcut();

protected:
	void keyPressEvent(QKeyEvent *e);

private:
	QComboBox *keysCombo;
};

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
	static int const nZoomRadios = 5;
    SettingsDialog(QWidget *parent);

private slots:
	void pickColor();
	void pickThumbsColor();
	void pickThumbsTextColor();
	void pickStartupDir();
	void setActionKeyText(const QString &text);

public slots:
	void abort();
	void saveSettings();

private:
	QRadioButton *fitLargeRadios[nZoomRadios];
	QRadioButton *fitSmallRadios[nZoomRadios];
	QCheckBox *compactLayoutCb;
	QToolButton *backgroundColorButton;
    QToolButton *colThumbButton;
    QToolButton *colThumbTextButton;
	QSpinBox *thumbSpacingSpin;
	QSpinBox *thumbPagesSpin;
	QSpinBox *saveQualitySpin;
	QColor bgColor;
	QColor thumbBgColor;
	QColor thumbTextColor;
	QCheckBox *exitCliCb;
	QCheckBox *wrapListCb;
	QCheckBox *enableAnimCb;
	QCheckBox *enableExifCb;
	QCheckBox *noSmallThumbCb;
	QCheckBox *reverseMouseCb;
	QSpinBox *slideDelaySpin;
	QCheckBox *slideRandomCb;
	KeyGrabLineEdit *keyLine;
	QRadioButton *startupDirRadios[3];
	QLineEdit *startupDirEdit;

	void setButtonBgColor(QColor &color, QToolButton *button);
};

/* Added by LTC */
class AutoDetectDialog : public QDialog
{
	Q_OBJECT
public:
    AutoDetectDialog(QWidget *parent);

private slots:
	void autoDetect();

private:
	QLabel *startTimeLabel;
	QLineEdit *startTimeEdit;
	QLabel *endTimeLabel;
	QLineEdit *endTimeEdit;
	QLabel *eLocLabel;
	QLabel *vLocLabel;
	QLabel *evLocLabel;
	QLineEdit *eLocEdit;
	QLineEdit *vLocEdit;
	QLineEdit *evLocEdit;
//	QLabel *showImageLabel;
	QWidget *showImageLabel;
	QPainter *painter;
	QPushButton *autoDetectButton;
	QTimer *timer;

	void imageDetect(string fileName);
	char coordinate_transformation(float x, float y);
};

/* Added by LTC */
class ElocDetectDialog : public QDialog
{
	Q_OBJECT;
public:
	ElocDetectDialog(QWidget *parent);

private slots:
	void elocDetect();
	void setClient();
//	void acceptConnection();
//	void readClient();
	void timerUpdate();
	void onChanged1(int);
	void onChanged2(int);
	void onChanged3(int);
	void onChanged4(int);

private:
//	QLineEdit *firstRouterEdit;
//	QLineEdit *secondRouterEdit;
//	QLineEdit *thirdRouterEdit;
//	QLineEdit *fourthRouterEdit;
	QComboBox *firstRouterBox;
	QComboBox *secondRouterBox;
	QComboBox *thirdRouterBox;
	QComboBox *fourthRouterBox;

//	QLineEdit *firstClientEdit;
	QTextEdit *firstClientEdit;
	QTextEdit *secondClientEdit;
	QTextEdit *thirdClientEdit;
	QTextEdit *fourthClientEdit;

	time_t start_time;

//	QTcpServer *server;
//	QTcpSocket *clientConnection;
	QTimer *clearTimer;

public:
//	QString room1_str, room2_str, room3_str, room4_str;
//	QString room1_client_str, room2_client_str, room3_client_str, room4_client_str;

};

/* Added by LTC */
class QElocServer : public QTcpServer
{
	Q_OBJECT
public:
	QElocServer(QObject *parent = 0);

private slots:
	void readOverToDialog();

private:
	void incomingConnection(qintptr socketDescriptor);

signals:
	void error(QTcpSocket::SocketError socketError);
	void readOver2();
};

/* Added by LTC */
class QElocClient : public QTcpSocket
{
	Q_OBJECT
public:
	QElocClient(QObject *parent = 0);

signals:
	void readOver();

private slots:
	void readClient();
};
/* Added by LTC */
class myLabel : public QWidget
{
	Q_OBJECT
public:
	myLabel();
private slots:
	void drawPoint(float x, float y);
protected:
	void paintEvent(QPaintEvent *event);
};

class CropDialog : public QDialog
{
    Q_OBJECT

public:
    CropDialog(QWidget *parent, ImageView *imageView);

public slots:
	void ok();
	void applyCrop(int);

private:
	QSpinBox *topSpin;
	QSpinBox *bottomSpin;
	QSpinBox *leftSpin;
	QSpinBox *rightSpin;
	ImageView *imageView;
};

class ColorsDialog : public QDialog
{
    Q_OBJECT

public:
    ColorsDialog(QWidget *parent, ImageView *imageView);

public slots:
	void ok();
	void reset();
	void enableHueSat(int state);
	void enableColorize(int state);
	void setRedChannel();
	void setGreenChannel();
	void setBlueChannel();
	void applyColors(int value);

private:
	ImageView *imageView;
	QCheckBox *hueSatEnabledCb;
	QSlider *hueSlide;
	QCheckBox *colorizeCb;
	QSlider *saturationSlide;
	QSlider *lightnessSlide;
	QCheckBox *redB;
	QCheckBox *greenB;
	QCheckBox *blueB;
};

class AppMgmtDialog : public QDialog
{
    Q_OBJECT

public:
    AppMgmtDialog(QWidget *parent);

public slots:
	void ok();

private slots:
	void add();
	void remove();

private:
	QTableView *appsTable;
	QStandardItemModel *appsTableModel;

	void addTableModelItem(QStandardItemModel *model, QString &key, QString &val);
};

class CopyMoveToDialog : public QDialog
{
    Q_OBJECT

public:
    CopyMoveToDialog(QWidget *parent, QString thumbsPath);
	QString selectedPath;
	bool copyOp;

private slots:
	void copy();
	void move();
	void copyOrMove(bool copy);
	void justClose();
	void add();
	void remove();

private:
	QTableView *pathsTable;
	QStandardItemModel *pathsTableModel;
	QString currentPath;

	void savePaths();
};

#endif // DIALOGS_H

