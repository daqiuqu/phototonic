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

#ifndef THUMBVIEW_H
#define THUMBVIEW_H

#include <QListView>
#include <QList>
#include <QDir>
#include <QDebug>
#include <QStandardItem>
#include <QTreeView>

class QListView;
class QStandardItemModel;

class ThumbView : public QListView
{
    Q_OBJECT

signals:
	void unsetBusy();
	void updateState(QString state);

public slots:
	void abort();
	void updateIndex();
	void handleSelectionChanged(const QItemSelection& selection);

public:
	ThumbView(QWidget *parent, int thumbSize);
	~ThumbView();
	void load();
	void setNeedScroll(bool needScroll)
	{
		m_needScroll = needScroll;
	}

	QList<QStandardItem*> *thumbList;
	QList<bool> *thumbIsLoaded;
	QStandardItemModel *thumbViewModel;
	QString currentViewDir;
	QDir::SortFlags thumbsSortFlags;
	int m_thumbSize;

protected:
    void startDrag(Qt::DropActions);
	
private:
	QFileInfoList thumbFileInfoList;
	QFileInfo thumbFileInfo;
	QDir *thumbsDir;
	bool abortOp;
	int newIndex;
	bool thumbLoaderActive;
	QStringList *m_fileFilters;
	bool m_needScroll;

	void initThumbs();
	void loadThumbs();
	int getFirstVisibleItem();
};

class FSTree : public QTreeView
{
    Q_OBJECT

public:
	FSTree(QWidget *parent);
	~FSTree();

protected:
	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);

signals:
	void dropOp(Qt::KeyboardModifiers keyMods, bool dirOp, QString cpMvDirPath);

private:
	QModelIndex dndOrigSelection;
	
};

#endif // THUMBVIEW_H
