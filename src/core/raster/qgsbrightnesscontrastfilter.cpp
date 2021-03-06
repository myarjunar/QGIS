/***************************************************************************
                         qgsbrightnesscontrastfilter.cpp
                         ---------------------
    begin                : February 2013
    copyright            : (C) 2013 by Alexander Bruy
    email                : alexander dot bruy at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrasterdataprovider.h"
#include "qgsbrightnesscontrastfilter.h"

#include <qmath.h>
#include <QDomDocument>
#include <QDomElement>

QgsBrightnessContrastFilter::QgsBrightnessContrastFilter( QgsRasterInterface *input )
  : QgsRasterInterface( input )
  , mBrightness( 0 )
  , mContrast( 0 )
{
}

QgsBrightnessContrastFilter *QgsBrightnessContrastFilter::clone() const
{
  QgsDebugMsgLevel( "Entered", 4 );
  QgsBrightnessContrastFilter *filter = new QgsBrightnessContrastFilter( nullptr );
  filter->setBrightness( mBrightness );
  filter->setContrast( mContrast );
  return filter;
}

int QgsBrightnessContrastFilter::bandCount() const
{
  if ( mOn )
  {
    return 1;
  }

  if ( mInput )
  {
    return mInput->bandCount();
  }

  return 0;
}

Qgis::DataType QgsBrightnessContrastFilter::dataType( int bandNo ) const
{
  if ( mOn )
  {
    return Qgis::ARGB32_Premultiplied;
  }

  if ( mInput )
  {
    return mInput->dataType( bandNo );
  }

  return Qgis::UnknownDataType;
}

bool QgsBrightnessContrastFilter::setInput( QgsRasterInterface *input )
{
  QgsDebugMsgLevel( "Entered", 4 );

  // Brightness filter can only work with single band ARGB32_Premultiplied
  if ( !input )
  {
    QgsDebugMsgLevel( "No input", 4 );
    return false;
  }

  if ( !mOn )
  {
    // In off mode we can connect to anything
    QgsDebugMsgLevel( "OK", 4 );
    mInput = input;
    return true;
  }

  if ( input->bandCount() < 1 )
  {
    QgsDebugMsg( "No input band" );
    return false;
  }

  if ( input->dataType( 1 ) != Qgis::ARGB32_Premultiplied &&
       input->dataType( 1 ) != Qgis::ARGB32 )
  {
    QgsDebugMsg( "Unknown input data type" );
    return false;
  }

  mInput = input;
  QgsDebugMsgLevel( "OK", 4 );
  return true;
}

QgsRasterBlock *QgsBrightnessContrastFilter::block( int bandNo, QgsRectangle  const &extent, int width, int height, QgsRasterBlockFeedback *feedback )
{
  Q_UNUSED( bandNo );
  QgsDebugMsgLevel( QString( "width = %1 height = %2 extent = %3" ).arg( width ).arg( height ).arg( extent.toString() ), 4 );

  QgsRasterBlock *outputBlock = new QgsRasterBlock();
  if ( !mInput )
  {
    return outputBlock;
  }

  // At this moment we know that we read rendered image
  int bandNumber = 1;
  QgsRasterBlock *inputBlock = mInput->block( bandNumber, extent, width, height, feedback );
  if ( !inputBlock || inputBlock->isEmpty() )
  {
    QgsDebugMsg( "No raster data!" );
    delete inputBlock;
    return outputBlock;
  }

  if ( mBrightness == 0 && mContrast == 0 )
  {
    QgsDebugMsgLevel( "No brightness changes.", 4 );
    delete outputBlock;
    return inputBlock;
  }

  if ( !outputBlock->reset( Qgis::ARGB32_Premultiplied, width, height ) )
  {
    delete inputBlock;
    return outputBlock;
  }

  // adjust image
  QRgb myNoDataColor = qRgba( 0, 0, 0, 0 );
  QRgb myColor;

  int r, g, b, alpha;
  double f = qPow( ( mContrast + 100 ) / 100.0, 2 );

  for ( qgssize i = 0; i < ( qgssize )width * height; i++ )
  {
    if ( inputBlock->color( i ) == myNoDataColor )
    {
      outputBlock->setColor( i, myNoDataColor );
      continue;
    }

    myColor = inputBlock->color( i );
    alpha = qAlpha( myColor );

    r = adjustColorComponent( qRed( myColor ), alpha, mBrightness, f );
    g = adjustColorComponent( qGreen( myColor ), alpha, mBrightness, f );
    b = adjustColorComponent( qBlue( myColor ), alpha, mBrightness, f );

    outputBlock->setColor( i, qRgba( r, g, b, alpha ) );
  }

  delete inputBlock;
  return outputBlock;
}

int QgsBrightnessContrastFilter::adjustColorComponent( int colorComponent, int alpha, int brightness, double contrastFactor ) const
{
  if ( alpha == 255 )
  {
    // Opaque pixel, do simpler math
    return qBound( 0, ( int )( ( ( ( ( ( colorComponent / 255.0 ) - 0.5 ) * contrastFactor ) + 0.5 ) * 255 ) + brightness ), 255 );
  }
  else if ( alpha == 0 )
  {
    // Totally transparent pixel
    return 0;
  }
  else
  {
    // Semi-transparent pixel. We need to adjust the math since we are using Qgis::ARGB32_Premultiplied
    // and color values have been premultiplied by alpha
    double alphaFactor = alpha / 255.;
    double adjustedColor = colorComponent / alphaFactor;

    // Make sure to return a premultiplied color
    return alphaFactor * qBound( 0., ( ( ( ( ( ( adjustedColor / 255.0 ) - 0.5 ) * contrastFactor ) + 0.5 ) * 255 ) + brightness ), 255. );
  }
}

void QgsBrightnessContrastFilter::writeXml( QDomDocument &doc, QDomElement &parentElem ) const
{
  if ( parentElem.isNull() )
  {
    return;
  }

  QDomElement filterElem = doc.createElement( QStringLiteral( "brightnesscontrast" ) );

  filterElem.setAttribute( QStringLiteral( "brightness" ), QString::number( mBrightness ) );
  filterElem.setAttribute( QStringLiteral( "contrast" ), QString::number( mContrast ) );
  parentElem.appendChild( filterElem );
}

void QgsBrightnessContrastFilter::readXml( const QDomElement &filterElem )
{
  if ( filterElem.isNull() )
  {
    return;
  }

  mBrightness = filterElem.attribute( QStringLiteral( "brightness" ), QStringLiteral( "0" ) ).toInt();
  mContrast = filterElem.attribute( QStringLiteral( "contrast" ), QStringLiteral( "0" ) ).toInt();
}
