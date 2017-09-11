/***************************************************************************
    qgsappbrowserdockwidgetmenuprovider.h
    ---------------------
    begin                : September 2017
    copyright            : (C) 2017 by Muhammad Yarjuna Rohmat
    email                : myarjunar at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSAPPBROWSERDOCKWIDGETMENUPROVIDER_H
#define QGSAPPBROWSERDOCKWIDGETMENUPROVIDER_H

#include <QObject>
#include <QMenu>

#include "qgsbrowserdockwidget.h"
#include "qgsmaplayer.h"

class QAction;

class QgsAppBrowserDockWidgetMenuProvider : public QObject, public QgsBrowserDockWidgetMenuProvider
{
    Q_OBJECT
  public:
    QgsAppBrowserDockWidgetMenuProvider( QgsBrowserDockWidget *widget );

    QMenu *createContextMenu( QgsDataItem *item ) override;

  protected:

    QgsBrowserDockWidget *mBrowserWidget = nullptr;

};

#endif // QGSAPPBROWSERDOCKWIDGETMENUPROVIDER_H
