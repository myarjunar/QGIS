/***************************************************************************
  qgsbrowserdockwidgetdefaultactions.cpp
  --------------------------------------
  Date                 : September 2017
  Copyright            : (C) 2017 by Muhammad Yarjuna Rohmat
  Email                : myarjunar at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsbrowserdockwidgetdefaultactions.h"
#include "qgsbrowserdockwidget.h"
#include "qgsbrowsermodel.h"
#include "qgssettings.h"
#include "qgslogger.h"

#include <QFileDialog>

class QgsBrowserModel;

QgsBrowserDockWidgetDefaultActions::QgsBrowserDockWidgetDefaultActions(QgsBrowserDockWidget *widget)
  : QObject( widget )
  , mBrowserWidget( widget )
  , mBrowserView( widget->browserView() )
  , mModel( widget->browserModel() )
  , mProxyModel( widget->proxyModel() )
{
}

QAction* QgsBrowserDockWidgetDefaultActions::actionAddFavorite(QObject *parent)
{
  QAction *a = new QAction( tr( "Add as a Favorite" ), parent );
  connect( a, &QAction::triggered, this, &QgsBrowserDockWidgetDefaultActions::addFavorite );
  return a;
}

QAction* QgsBrowserDockWidgetDefaultActions::actionAddFavoriteDirectory(QObject *parent)
{
  QAction *a = new QAction( tr( "Add a Directory..." ), parent );
  connect( a, &QAction::triggered, this, &QgsBrowserDockWidgetDefaultActions::addFavoriteDirectory );
  return a;
}

QAction* QgsBrowserDockWidgetDefaultActions::actionRemoveFavorite(QObject *parent)
{
  QAction *a = new QAction( tr( "Remove Favorite" ), parent );
  connect( a, &QAction::triggered, this, &QgsBrowserDockWidgetDefaultActions::removeFavorite );
  return a;
}

QAction* QgsBrowserDockWidgetDefaultActions::actionAddSelectedLayers(QObject *parent)
{
  QAction *a = new QAction( tr( "Add Selected Layer(s) to Canvas" ), parent );
  connect( a, &QAction::triggered, this, &QgsBrowserDockWidgetDefaultActions::addSelectedLayers );
  return a;
}

QAction* QgsBrowserDockWidgetDefaultActions::actionShowProperties(QObject *parent)
{
  QAction *a = new QAction( tr( "Properties..." ), parent );
  connect( a, &QAction::triggered, this, &QgsBrowserDockWidgetDefaultActions::showProperties );
  return a;
}

QAction* QgsBrowserDockWidgetDefaultActions::actionHideItem(QObject *parent)
{
  QAction *a = new QAction( tr( "Hide from Browser" ), parent );
  connect( a, &QAction::triggered, this, &QgsBrowserDockWidgetDefaultActions::hideItem );
  return a;
}

QAction* QgsBrowserDockWidgetDefaultActions::actionToggleFastScan(QObject *parent)
{
  QAction *a = new QAction( tr( "Fast Scan this Directory" ), parent );
  connect( a, &QAction::triggered, this, &QgsBrowserDockWidgetDefaultActions::toggleFastScan );
  return a;
}

void QgsBrowserDockWidgetDefaultActions::addFavorite()
{
  QModelIndex index = mProxyModel->mapToSource( mBrowserView->currentIndex() );
  QgsDataItem *item = mModel->dataItem( index );
  if ( !item )
    return;

  QgsDirectoryItem *dirItem = dynamic_cast<QgsDirectoryItem *>( item );
  if ( !dirItem )
    return;

  mModel->addFavoriteDirectory( dirItem->dirPath() );
}

void QgsBrowserDockWidgetDefaultActions::addFavoriteDirectory()
{
  QString directory = QFileDialog::getExistingDirectory( mBrowserWidget, tr( "Add directory to favorites" ) );
  if ( !directory.isEmpty() )
  {
    mModel->addFavoriteDirectory( directory );
  }
}

void QgsBrowserDockWidgetDefaultActions::removeFavorite()
{
  mModel->removeFavorite( mProxyModel->mapToSource( mBrowserView->currentIndex() ) );
}

void QgsBrowserDockWidgetDefaultActions::addLayer(QgsLayerItem *layerItem)
{
  if ( !layerItem )
    return;

  QgsMimeDataUtils::UriList list;
  list << layerItem->mimeUri();
  emit mBrowserWidget->handleDropUriList( list );
}

void QgsBrowserDockWidgetDefaultActions::addSelectedLayers()
{
  QgsDebugMsg( " ASU LAYER " );
  QApplication::setOverrideCursor( Qt::WaitCursor );

  // get a sorted list of selected indexes
  QModelIndexList list = mBrowserView->selectionModel()->selectedIndexes();
  std::sort( list.begin(), list.end() );

  QgsDebugMsg( QString(" ASU LIST COUNT %1 ").arg( list.size() ) );

  // If any of the layer items are QGIS we just open and exit the loop
  Q_FOREACH ( const QModelIndex &index, list )
    {
      QgsDataItem *item = mModel->dataItem( mProxyModel->mapToSource( index ) );
      if ( item && item->type() == QgsDataItem::Project )
      {
        QgsProjectItem *projectItem = qobject_cast<QgsProjectItem *>( item );
        if ( projectItem )
            emit mBrowserWidget->openFile( projectItem->path() );

        QApplication::restoreOverrideCursor();
        return;
      }
    }

  // add items in reverse order so they are in correct order in the layers dock
  for ( int i = list.size() - 1; i >= 0; i-- )
  {
    QgsDataItem *item = mModel->dataItem( mProxyModel->mapToSource( list[i] ) );
    if ( item && item->type() == QgsDataItem::Layer )
    {
      QgsLayerItem *layerItem = qobject_cast<QgsLayerItem *>( item );
      if ( layerItem )
      {
        QgsDebugMsg( "ASU LAYERITEM " );
        addLayer( layerItem );
      }
    }
  }

  QApplication::restoreOverrideCursor();
}

void QgsBrowserDockWidgetDefaultActions::hideItem()
{
  QModelIndex index = mProxyModel->mapToSource( mBrowserView->currentIndex() );
  QgsDataItem *item = mModel->dataItem( index );
  if ( ! item )
    return;

  if ( item->type() == QgsDataItem::Directory )
  {
    mModel->hidePath( item );
  }
}

void QgsBrowserDockWidgetDefaultActions::showProperties()
{
  QModelIndex index = mProxyModel->mapToSource( mBrowserView->currentIndex() );
  QgsDataItem *item = mModel->dataItem( index );
  if ( ! item )
    return;

  if ( item->type() == QgsDataItem::Layer || item->type() == QgsDataItem::Directory )
  {
    QgsBrowserPropertiesDialog *dialog = new QgsBrowserPropertiesDialog( settingsSection(), mBrowserWidget );
    dialog->setItem( item );
    dialog->show();
  }
}

void QgsBrowserDockWidgetDefaultActions::toggleFastScan()
{
  QModelIndex index = mProxyModel->mapToSource( mBrowserView->currentIndex() );
  QgsDataItem *item = mModel->dataItem( index );
  if ( ! item )
    return;

  if ( item->type() == QgsDataItem::Directory )
  {
    QgsSettings settings;
    QStringList fastScanDirs = settings.value( QStringLiteral( "qgis/scanItemsFastScanUris" ),
                                               QStringList() ).toStringList();
    int idx = fastScanDirs.indexOf( item->path() );
    if ( idx != -1 )
    {
      fastScanDirs.removeAt( idx );
    }
    else
    {
      fastScanDirs << item->path();
    }
    settings.setValue( QStringLiteral( "qgis/scanItemsFastScanUris" ), fastScanDirs );
  }
}
