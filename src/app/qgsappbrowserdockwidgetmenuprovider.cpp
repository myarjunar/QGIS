/***************************************************************************
    qgsappbrowserdockwidgetmenuprovider.cpp
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

#include "qgisapp.h"
#include "qgsappbrowserdockwidgetmenuprovider.h"
#include "qgsclipboard.h"
#include "qgssettings.h"

QgsAppBrowserDockWidgetMenuProvider::QgsAppBrowserDockWidgetMenuProvider(QgsBrowserDockWidget *widget)
   : mBrowserWidget( widget )
{
}

QMenu *QgsAppBrowserDockWidgetMenuProvider::createContextMenu( QgsDataItem *item )
{
  QMenu *menu = new QMenu;

  if ( !item )
    return menu;

  QgsBrowserDockWidgetDefaultActions *defaultActions = mBrowserWidget->defaultActions();

  if ( item->type() == QgsDataItem::Directory )
  {
    QgsSettings settings;
    QStringList favDirs = settings.value( QStringLiteral( "browser/favourites" ) ).toStringList();
    bool inFavDirs = item->parent() && item->parent()->type() == QgsDataItem::Favorites;

    if ( item->parent() && !inFavDirs )
    {
      // only non-root directories can be added as favorites
      menu->addAction( defaultActions->actionAddFavorite() );
    }
    else if ( inFavDirs )
    {
      // only favorites can be removed
      menu->addAction( defaultActions->actionRemoveFavorite() );
    }
    menu->addAction( defaultActions->actionShowProperties() );
    menu->addAction( defaultActions->actionHideItem() );

    QAction *action = defaultActions->actionToggleFastScan();
    action->setCheckable( true );
    action->setChecked( settings.value( QStringLiteral( "qgis/scanItemsFastScanUris" ),
                                        QStringList() ).toStringList().contains( item->path() ) );
    menu->addAction( action );
  }
  else if ( item->type() == QgsDataItem::Layer )
  {
    menu->addAction( defaultActions->actionAddSelectedLayers() );
    menu->addAction( defaultActions->actionShowProperties() );
    if ( item->path().contains( QStringLiteral( "geonode" ) ) )
    {
      QMenu *menuStyleManager = new QMenu( tr( "Styles" ), menu );

      QgisApp *app = QgisApp::instance();
      menuStyleManager->addAction( tr( "Copy Style" ), app, SLOT( getStyle() ) );
      if ( app->clipboard()->hasFormat( QGSCLIPBOARD_STYLE_MIME ) )
      {
        menuStyleManager->addAction( tr( "Paste Style" ), app, SLOT( pasteStyle() ) );
      }

      menu->addMenu( menuStyleManager );
    }
  }
  else if ( item->type() == QgsDataItem::Favorites )
  {
    menu->addAction( defaultActions->actionAddFavoriteDirectory() );
  }

  QList<QAction *> actions = item->actions();
  if ( !actions.isEmpty() )
  {
    if ( !menu->actions().isEmpty() )
      menu->addSeparator();
    // add action to the menu
    menu->addActions( actions );
  }

  return menu;
}
