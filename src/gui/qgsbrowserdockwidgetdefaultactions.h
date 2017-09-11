/***************************************************************************
  qgsbrowserdockwidgetdefaultactions.h
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

#ifndef QGSBROWSERDOCKWIDGETDEFAULTACTIONS_H
#define QGSBROWSERDOCKWIDGETDEFAULTACTIONS_H

#include <QObject>
#include "qgis.h"
#include "qgis_gui.h"
#include "qgsbrowserdockwidget.h"

class QgsBrowserDockWidget;

/**
 * \ingroup gui
 * The QgsBrowserDockWidgetDefaultActions class serves as a factory of actions
 * that can be used together with a browser dock widget.
 *
 * \see QgsBrowserDockWidget
 * \since QGIS 3.0
 */

class GUI_EXPORT QgsBrowserDockWidgetDefaultActions : public QObject
{
    Q_OBJECT
  public:
    QgsBrowserDockWidgetDefaultActions( QgsBrowserDockWidget *widget );

    //! Action to add selected directory as a favorite directory
    QAction *actionAddFavorite( QObject *parent = nullptr ) SIP_FACTORY;

    //! Action to remove selected directory from favorite
    QAction *actionRemoveFavorite( QObject *parent = nullptr ) SIP_FACTORY;

    //! Action to show properties of selected item
    QAction *actionShowProperties( QObject *parent = nullptr ) SIP_FACTORY;

    //! Action to hide selected item from the browser view
    QAction *actionHideItem( QObject *parent = nullptr ) SIP_FACTORY;

    //! Action to dast scan selected directory
    QAction *actionToggleFastScan( QObject *parent = nullptr ) SIP_FACTORY;

    //! Action to add selected layer(s) to map canvas
    QAction *actionAddSelectedLayers( QObject *parent = nullptr ) SIP_FACTORY;

    //! Action to add favorite directory to favorite item
    QAction *actionAddFavoriteDirectory( QObject *parent = nullptr ) SIP_FACTORY;

  public slots:
    //! Add current item to favorite
    void addFavorite();
    //! Add directory from file dialog to favorite
    void addFavoriteDirectory();
    //! Remove from favorite
    void removeFavorite();

    //! Add selected layers to the project
    void addSelectedLayers();
    //! Show the layer properties
    void showProperties();
    //! Hide current item
    void hideItem();
    //! Toggle fast scan
    void toggleFastScan();

  signals:
    //! Emitted when a file needs to be opened
    void openFile( const QString & );
    //! Emitted when drop uri list needs to be handled
    void handleDropUriList( const QgsMimeDataUtils::UriList & );

  protected:
    QgsBrowserDockWidget *mBrowserWidget;
    QgsDockBrowserTreeView *mBrowserView;
    QgsBrowserModel *mModel;
    QgsBrowserTreeFilterProxyModel *mProxyModel;

  private:
    //! Add a layer
    void addLayer( QgsLayerItem *layerItem );
    //! Settings prefix (the object name)
    QString settingsSection() { return objectName().toLower(); }

};

#endif // QGSBROWSERDOCKWIDGETDEFAULTACTIONS_H
