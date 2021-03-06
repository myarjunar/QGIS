/***************************************************************************
 *  qgsgeometrysnapper.cpp                                                 *
 *  -------------------                                                    *
 *  copyright            : (C) 2014 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QtConcurrentMap>
#include <qmath.h>

#include "qgsfeatureiterator.h"
#include "qgsgeometry.h"
#include "qgsvectorlayer.h"
#include "qgsgeometrysnapper.h"
#include "qgsvectordataprovider.h"
#include "qgsgeometryutils.h"
#include "qgsmapsettings.h"
#include "qgssurface.h"
#include "qgscurve.h"

///@cond PRIVATE

QgsSnapIndex::PointSnapItem::PointSnapItem( const QgsSnapIndex::CoordIdx *_idx )
  : SnapItem( QgsSnapIndex::SnapPoint )
  , idx( _idx )
{}

QgsPointV2 QgsSnapIndex::PointSnapItem::getSnapPoint( const QgsPointV2 &/*p*/ ) const
{
  return idx->point();
}

QgsSnapIndex::SegmentSnapItem::SegmentSnapItem( const QgsSnapIndex::CoordIdx *_idxFrom, const QgsSnapIndex::CoordIdx *_idxTo )
  : SnapItem( QgsSnapIndex::SnapSegment )
  , idxFrom( _idxFrom )
  , idxTo( _idxTo )
{}

QgsPointV2 QgsSnapIndex::SegmentSnapItem::getSnapPoint( const QgsPointV2 &p ) const
{
  return QgsGeometryUtils::projPointOnSegment( p, idxFrom->point(), idxTo->point() );
}

bool QgsSnapIndex::SegmentSnapItem::getIntersection( const QgsPointV2 &p1, const QgsPointV2 &p2, QgsPointV2 &inter ) const
{
  const QgsPointV2 &q1 = idxFrom->point(), & q2 = idxTo->point();
  QgsVector v( p2.x() - p1.x(), p2.y() - p1.y() );
  QgsVector w( q2.x() - q1.x(), q2.y() - q1.y() );
  double vl = v.length();
  double wl = w.length();

  if ( qFuzzyIsNull( vl ) || qFuzzyIsNull( wl ) )
  {
    return false;
  }
  v = v / vl;
  w = w / wl;

  double d = v.y() * w.x() - v.x() * w.y();

  if ( d == 0 )
    return false;

  double dx = q1.x() - p1.x();
  double dy = q1.y() - p1.y();
  double k = ( dy * w.x() - dx * w.y() ) / d;

  inter = QgsPointV2( p1.x() + v.x() * k, p1.y() + v.y() * k );

  double lambdav = QgsVector( inter.x() - p1.x(), inter.y() - p1.y() ) *  v;
  if ( lambdav < 0. + 1E-8 || lambdav > vl - 1E-8 )
    return false;

  double lambdaw = QgsVector( inter.x() - q1.x(), inter.y() - q1.y() ) * w;
  if ( lambdaw < 0. + 1E-8 || lambdaw >= wl - 1E-8 )
    return false;

  return true;
}

bool QgsSnapIndex::SegmentSnapItem::getProjection( const QgsPointV2 &p, QgsPointV2 &pProj )
{
  const QgsPointV2 &s1 = idxFrom->point();
  const QgsPointV2 &s2 = idxTo->point();
  double nx = s2.y() - s1.y();
  double ny = -( s2.x() - s1.x() );
  double t = ( p.x() * ny - p.y() * nx - s1.x() * ny + s1.y() * nx ) / ( ( s2.x() - s1.x() ) * ny - ( s2.y() - s1.y() ) * nx );
  if ( t < 0. || t > 1. )
  {
    return false;
  }
  pProj = QgsPointV2( s1.x() + ( s2.x() - s1.x() ) * t, s1.y() + ( s2.y() - s1.y() ) * t );
  return true;
}

///////////////////////////////////////////////////////////////////////////////

class Raytracer
{
    // Raytrace on an integer, unit-width 2D grid with floating point coordinates
    // See http://playtechs.blogspot.ch/2007/03/raytracing-on-grid.html
  public:
    Raytracer( float x0, float y0, float x1, float y1 )
      : m_dx( qAbs( x1 - x0 ) )
      , m_dy( qAbs( y1 - y0 ) )
      , m_x( qFloor( x0 ) )
      , m_y( qFloor( y0 ) )
      , m_n( 1 )
    {
      if ( m_dx == 0. )
      {
        m_xInc = 0.;
        m_error = std::numeric_limits<float>::infinity();
      }
      else if ( x1 > x0 )
      {
        m_xInc = 1;
        m_n += int( qFloor( x1 ) ) - m_x;
        m_error = ( qFloor( x0 ) + 1 - x0 ) * m_dy;
      }
      else
      {
        m_xInc = -1;
        m_n += m_x - int( qFloor( x1 ) );
        m_error = ( x0 - qFloor( x0 ) ) * m_dy;
      }
      if ( m_dy == 0. )
      {
        m_yInc = 0.;
        m_error = -std::numeric_limits<float>::infinity();
      }
      else if ( y1 > y0 )
      {
        m_yInc = 1;
        m_n += int( qFloor( y1 ) ) - m_y;
        m_error -= ( qFloor( y0 ) + 1 - y0 ) * m_dx;
      }
      else
      {
        m_yInc = -1;
        m_n += m_y - int( qFloor( y1 ) );
        m_error -= ( y0 - qFloor( y0 ) ) * m_dx;
      }
    }
    int curCol() const { return m_x; }
    int curRow() const { return m_y; }
    void next()
    {
      if ( m_error > 0 )
      {
        m_y += m_yInc;
        m_error -= m_dx;
      }
      else if ( m_error < 0 )
      {
        m_x += m_xInc;
        m_error += m_dy;
      }
      else
      {
        m_x += m_xInc;
        m_y += m_yInc;
        m_error += m_dx;
        m_error -= m_dy;
        --m_n;
      }
      --m_n;
    }

    bool isValid() const { return m_n > 0; }

  private:
    float m_dx, m_dy;
    int m_x, m_y;
    int m_xInc, m_yInc;
    float m_error;
    int m_n;
};

///////////////////////////////////////////////////////////////////////////////

QgsSnapIndex::GridRow::~GridRow()
{
  Q_FOREACH ( const QgsSnapIndex::Cell &cell, mCells )
  {
    qDeleteAll( cell );
  }
}

QgsSnapIndex::Cell &QgsSnapIndex::GridRow::getCreateCell( int col )
{
  if ( col < mColStartIdx )
  {
    for ( int i = col; i < mColStartIdx; ++i )
    {
      mCells.prepend( Cell() );
    }
    mColStartIdx = col;
    return mCells.front();
  }
  else if ( col >= mColStartIdx + mCells.size() )
  {
    for ( int i = mColStartIdx + mCells.size(); i <= col; ++i )
    {
      mCells.append( Cell() );
    }
    return mCells.back();
  }
  else
  {
    return mCells[col - mColStartIdx];
  }
}

const QgsSnapIndex::Cell *QgsSnapIndex::GridRow::getCell( int col ) const
{
  if ( col < mColStartIdx || col >= mColStartIdx + mCells.size() )
  {
    return nullptr;
  }
  else
  {
    return &mCells[col - mColStartIdx];
  }
}

QList<QgsSnapIndex::SnapItem *> QgsSnapIndex::GridRow::getSnapItems( int colStart, int colEnd ) const
{
  colStart = qMax( colStart, mColStartIdx );
  colEnd = qMin( colEnd, mColStartIdx + mCells.size() - 1 );

  QList<SnapItem *> items;

  for ( int col = colStart; col <= colEnd; ++col )
  {
    items.append( mCells[col - mColStartIdx] );
  }
  return items;
}

///////////////////////////////////////////////////////////////////////////////

QgsSnapIndex::QgsSnapIndex( const QgsPointV2 &origin, double cellSize )
  : mOrigin( origin )
  , mCellSize( cellSize )
  , mRowsStartIdx( 0 )
{
}

QgsSnapIndex::~QgsSnapIndex()
{
  qDeleteAll( mCoordIdxs );
}


const QgsSnapIndex::Cell *QgsSnapIndex::getCell( int col, int row ) const
{
  if ( row < mRowsStartIdx || row >= mRowsStartIdx + mGridRows.size() )
  {
    return nullptr;
  }
  else
  {
    return mGridRows[row - mRowsStartIdx].getCell( col );
  }
}

QgsSnapIndex::Cell &QgsSnapIndex::getCreateCell( int col, int row )
{
  if ( row < mRowsStartIdx )
  {
    for ( int i = row; i < mRowsStartIdx; ++i )
    {
      mGridRows.prepend( GridRow() );
    }
    mRowsStartIdx = row;
    return mGridRows.front().getCreateCell( col );
  }
  else if ( row >= mRowsStartIdx + mGridRows.size() )
  {
    for ( int i = mRowsStartIdx + mGridRows.size(); i <= row; ++i )
    {
      mGridRows.append( GridRow() );
    }
    return mGridRows.back().getCreateCell( col );
  }
  else
  {
    return mGridRows[row - mRowsStartIdx].getCreateCell( col );
  }
}

void QgsSnapIndex::addPoint( const CoordIdx *idx )
{
  QgsPointV2 p = idx->point();
  int col = qFloor( ( p.x() - mOrigin.x() ) / mCellSize );
  int row = qFloor( ( p.y() - mOrigin.y() ) / mCellSize );
  getCreateCell( col, row ).append( new PointSnapItem( idx ) );
}

void QgsSnapIndex::addSegment( const CoordIdx *idxFrom, const CoordIdx *idxTo )
{
  QgsPointV2 pFrom = idxFrom->point();
  QgsPointV2 pTo = idxTo->point();
  // Raytrace along the grid, get touched cells
  float x0 = ( pFrom.x() - mOrigin.x() ) / mCellSize;
  float y0 = ( pFrom.y() - mOrigin.y() ) / mCellSize;
  float x1 = ( pTo.x() - mOrigin.x() ) / mCellSize;
  float y1 = ( pTo.y() - mOrigin.y() ) / mCellSize;

  Raytracer rt( x0, y0, x1, y1 );
  for ( ; rt.isValid(); rt.next() )
  {
    getCreateCell( rt.curCol(), rt.curRow() ).append( new SegmentSnapItem( idxFrom, idxTo ) );
  }
}

void QgsSnapIndex::addGeometry( const QgsAbstractGeometry *geom )
{
  for ( int iPart = 0, nParts = geom->partCount(); iPart < nParts; ++iPart )
  {
    for ( int iRing = 0, nRings = geom->ringCount( iPart ); iRing < nRings; ++iRing )
    {
      int nVerts = geom->vertexCount( iPart, iRing );

      if ( dynamic_cast< const QgsSurface * >( geom ) )
        nVerts--;
      else if ( const QgsCurve *curve = dynamic_cast< const QgsCurve * >( geom ) )
      {
        if ( curve->isClosed() )
          nVerts--;
      }

      for ( int iVert = 0; iVert < nVerts; ++iVert )
      {
        CoordIdx *idx = new CoordIdx( geom, QgsVertexId( iPart, iRing, iVert ) );
        CoordIdx *idx1 = new CoordIdx( geom, QgsVertexId( iPart, iRing, iVert + 1 ) );
        mCoordIdxs.append( idx );
        mCoordIdxs.append( idx1 );
        addPoint( idx );
        if ( iVert < nVerts - 1 )
          addSegment( idx, idx1 );
      }
    }
  }
}


QgsPointV2 QgsSnapIndex::getClosestSnapToPoint( const QgsPointV2 &p, const QgsPointV2 &q )
{
  // Look for intersections on segment from the target point to the point opposite to the point reference point
  // p2 =  p1 + 2 * (q - p1)
  QgsPointV2 p2( 2 * q.x() - p.x(), 2 * q.y() - p.y() );

  // Raytrace along the grid, get touched cells
  float x0 = ( p.x() - mOrigin.x() ) / mCellSize;
  float y0 = ( p.y() - mOrigin.y() ) / mCellSize;
  float x1 = ( p2.x() - mOrigin.x() ) / mCellSize;
  float y1 = ( p2.y() - mOrigin.y() ) / mCellSize;

  Raytracer rt( x0, y0, x1, y1 );
  double dMin = std::numeric_limits<double>::max();
  QgsPointV2 pMin = p;
  for ( ; rt.isValid(); rt.next() )
  {
    const Cell *cell = getCell( rt.curCol(), rt.curRow() );
    if ( !cell )
    {
      continue;
    }
    Q_FOREACH ( const SnapItem *item, *cell )
    {
      if ( item->type == SnapSegment )
      {
        QgsPointV2 inter;
        if ( static_cast<const SegmentSnapItem *>( item )->getIntersection( p, p2, inter ) )
        {
          double dist = QgsGeometryUtils::sqrDistance2D( q, inter );
          if ( dist < dMin )
          {
            dMin = dist;
            pMin = inter;
          }
        }
      }
    }
  }

  return pMin;
}

QgsSnapIndex::SnapItem *QgsSnapIndex::getSnapItem( const QgsPointV2 &pos, double tol, QgsSnapIndex::PointSnapItem **pSnapPoint, QgsSnapIndex::SegmentSnapItem **pSnapSegment ) const
{
  int colStart = qFloor( ( pos.x() - tol - mOrigin.x() ) / mCellSize );
  int rowStart = qFloor( ( pos.y() - tol - mOrigin.y() ) / mCellSize );
  int colEnd = qFloor( ( pos.x() + tol - mOrigin.x() ) / mCellSize );
  int rowEnd = qFloor( ( pos.y() + tol - mOrigin.y() ) / mCellSize );

  rowStart = qMax( rowStart, mRowsStartIdx );
  rowEnd = qMin( rowEnd, mRowsStartIdx + mGridRows.size() - 1 );

  QList<SnapItem *> items;
  for ( int row = rowStart; row <= rowEnd; ++row )
  {
    items.append( mGridRows[row - mRowsStartIdx].getSnapItems( colStart, colEnd ) );
  }

  double minDistSegment = std::numeric_limits<double>::max();
  double minDistPoint = std::numeric_limits<double>::max();
  QgsSnapIndex::SegmentSnapItem *snapSegment = nullptr;
  QgsSnapIndex::PointSnapItem *snapPoint = nullptr;

  Q_FOREACH ( QgsSnapIndex::SnapItem *item, items )
  {
    if ( item->type == SnapPoint )
    {
      double dist = QgsGeometryUtils::sqrDistance2D( item->getSnapPoint( pos ), pos );
      if ( dist < minDistPoint )
      {
        minDistPoint = dist;
        snapPoint = static_cast<PointSnapItem *>( item );
      }
    }
    else if ( item->type == SnapSegment )
    {
      QgsPointV2 pProj;
      if ( !static_cast<SegmentSnapItem *>( item )->getProjection( pos, pProj ) )
      {
        continue;
      }
      double dist = QgsGeometryUtils::sqrDistance2D( pProj, pos );
      if ( dist < minDistSegment )
      {
        minDistSegment = dist;
        snapSegment = static_cast<SegmentSnapItem *>( item );
      }
    }
  }
  snapPoint = minDistPoint < tol * tol ? snapPoint : nullptr;
  snapSegment = minDistSegment < tol * tol ? snapSegment : nullptr;
  if ( pSnapPoint ) *pSnapPoint = snapPoint;
  if ( pSnapSegment ) *pSnapSegment = snapSegment;
  return minDistPoint < minDistSegment ? static_cast<QgsSnapIndex::SnapItem *>( snapPoint ) : static_cast<QgsSnapIndex::SnapItem *>( snapSegment );
}

/// @endcond




QgsGeometrySnapper::QgsGeometrySnapper( QgsVectorLayer *referenceLayer )
  : mReferenceLayer( referenceLayer )
{
  // Build spatial index
  QgsFeatureRequest req;
  req.setSubsetOfAttributes( QgsAttributeList() );
  mIndex = QgsSpatialIndex( mReferenceLayer->getFeatures( req ) );
}

QgsFeatureList QgsGeometrySnapper::snapFeatures( const QgsFeatureList &features, double snapTolerance, SnapMode mode )
{
  QgsFeatureList list = features;
  QtConcurrent::blockingMap( list, ProcessFeatureWrapper( this, snapTolerance, mode ) );
  return list;
}

void QgsGeometrySnapper::processFeature( QgsFeature &feature, double snapTolerance, SnapMode mode )
{
  if ( !feature.geometry().isNull() )
    feature.setGeometry( snapGeometry( feature.geometry(), snapTolerance, mode ) );
}

QgsGeometry QgsGeometrySnapper::snapGeometry( const QgsGeometry &geometry, double snapTolerance, SnapMode mode ) const
{
  QgsPointV2 center = dynamic_cast< const QgsPointV2 * >( geometry.geometry() ) ? *static_cast< const QgsPointV2 * >( geometry.geometry() ) :
                      QgsPointV2( geometry.geometry()->boundingBox().center() );

  // Get potential reference features and construct snap index
  QList<QgsGeometry> refGeometries;
  mIndexMutex.lock();
  QgsRectangle searchBounds = geometry.boundingBox();
  searchBounds.grow( snapTolerance );
  QgsFeatureIds refFeatureIds = mIndex.intersects( searchBounds ).toSet();
  mIndexMutex.unlock();

  QgsFeatureRequest refFeatureRequest = QgsFeatureRequest().setFilterFids( refFeatureIds ).setSubsetOfAttributes( QgsAttributeList() );
  mReferenceLayerMutex.lock();
  QgsFeature refFeature;
  QgsFeatureIterator refFeatureIt = mReferenceLayer->getFeatures( refFeatureRequest );

  while ( refFeatureIt.nextFeature( refFeature ) )
  {
    refGeometries.append( refFeature.geometry() );
  }
  mReferenceLayerMutex.unlock();


  QgsSnapIndex refSnapIndex( center, 10 * snapTolerance );
  Q_FOREACH ( const QgsGeometry &geom, refGeometries )
  {
    refSnapIndex.addGeometry( geom.geometry() );
  }

  // Snap geometries
  QgsAbstractGeometry *subjGeom = geometry.geometry()->clone();
  QList < QList< QList<PointFlag> > > subjPointFlags;

  // Pass 1: snap vertices of subject geometry to reference vertices
  for ( int iPart = 0, nParts = subjGeom->partCount(); iPart < nParts; ++iPart )
  {
    subjPointFlags.append( QList< QList<PointFlag> >() );

    for ( int iRing = 0, nRings = subjGeom->ringCount( iPart ); iRing < nRings; ++iRing )
    {
      subjPointFlags[iPart].append( QList<PointFlag>() );

      for ( int iVert = 0, nVerts = polyLineSize( subjGeom, iPart, iRing ); iVert < nVerts; ++iVert )
      {

        QgsSnapIndex::PointSnapItem *snapPoint = nullptr;
        QgsSnapIndex::SegmentSnapItem *snapSegment = nullptr;
        QgsVertexId vidx( iPart, iRing, iVert );
        QgsPointV2 p = subjGeom->vertexAt( vidx );
        if ( !refSnapIndex.getSnapItem( p, snapTolerance, &snapPoint, &snapSegment ) )
        {
          subjPointFlags[iPart][iRing].append( Unsnapped );
        }
        else
        {
          switch ( mode )
          {
            case PreferNodes:
            {
              // Prefer snapping to point
              if ( snapPoint )
              {
                subjGeom->moveVertex( vidx, snapPoint->getSnapPoint( p ) );
                subjPointFlags[iPart][iRing].append( SnappedToRefNode );
              }
              else if ( snapSegment )
              {
                subjGeom->moveVertex( vidx, snapSegment->getSnapPoint( p ) );
                subjPointFlags[iPart][iRing].append( SnappedToRefSegment );
              }
              break;
            }

            case PreferClosest:
            {
              QgsPointV2 nodeSnap, segmentSnap;
              double distanceNode = DBL_MAX;
              double distanceSegment = DBL_MAX;
              if ( snapPoint )
              {
                nodeSnap = snapPoint->getSnapPoint( p );
                distanceNode = nodeSnap.distanceSquared( p );
              }
              if ( snapSegment )
              {
                segmentSnap = snapSegment->getSnapPoint( p );
                distanceSegment = segmentSnap.distanceSquared( p );
              }
              if ( snapPoint && distanceNode < distanceSegment )
              {
                subjGeom->moveVertex( vidx, nodeSnap );
                subjPointFlags[iPart][iRing].append( SnappedToRefNode );
              }
              else if ( snapSegment )
              {
                subjGeom->moveVertex( vidx, segmentSnap );
                subjPointFlags[iPart][iRing].append( SnappedToRefSegment );
              }
              break;
            }
          }
        }
      }
    }
  }

  //nothing more to do for points
  if ( dynamic_cast< const QgsPointV2 * >( subjGeom ) )
    return QgsGeometry( subjGeom );

  // SnapIndex for subject feature
  std::unique_ptr< QgsSnapIndex > subjSnapIndex( new QgsSnapIndex( center, 10 * snapTolerance ) );
  subjSnapIndex->addGeometry( subjGeom );

  std::unique_ptr< QgsAbstractGeometry > origSubjGeom( subjGeom->clone() );
  std::unique_ptr< QgsSnapIndex > origSubjSnapIndex( new QgsSnapIndex( center, 10 * snapTolerance ) );
  origSubjSnapIndex->addGeometry( origSubjGeom.get() );

  // Pass 2: add missing vertices to subject geometry
  Q_FOREACH ( const QgsGeometry &refGeom, refGeometries )
  {
    for ( int iPart = 0, nParts = refGeom.geometry()->partCount(); iPart < nParts; ++iPart )
    {
      for ( int iRing = 0, nRings = refGeom.geometry()->ringCount( iPart ); iRing < nRings; ++iRing )
      {
        for ( int iVert = 0, nVerts = polyLineSize( refGeom.geometry(), iPart, iRing ); iVert < nVerts; ++iVert )
        {

          QgsSnapIndex::PointSnapItem *snapPoint = nullptr;
          QgsSnapIndex::SegmentSnapItem *snapSegment = nullptr;
          QgsPointV2 point = refGeom.geometry()->vertexAt( QgsVertexId( iPart, iRing, iVert ) );
          if ( subjSnapIndex->getSnapItem( point, snapTolerance, &snapPoint, &snapSegment ) )
          {
            // Snap to segment, unless a subject point was already snapped to the reference point
            if ( snapPoint && QgsGeometryUtils::sqrDistance2D( snapPoint->getSnapPoint( point ), point ) < 1E-16 )
            {
              continue;
            }
            else if ( snapSegment )
            {
              // Look if there is a closer reference segment, if so, ignore this point
              QgsPointV2 pProj = snapSegment->getSnapPoint( point );
              QgsPointV2 closest = refSnapIndex.getClosestSnapToPoint( point, pProj );
              if ( QgsGeometryUtils::sqrDistance2D( pProj, point ) > QgsGeometryUtils::sqrDistance2D( pProj, closest ) )
              {
                continue;
              }

              // If we are too far away from the original geometry, do nothing
              if ( !origSubjSnapIndex->getSnapItem( point, snapTolerance ) )
              {
                continue;
              }

              const QgsSnapIndex::CoordIdx *idx = snapSegment->idxFrom;
              subjGeom->insertVertex( QgsVertexId( idx->vidx.part, idx->vidx.ring, idx->vidx.vertex + 1 ), point );
              subjPointFlags[idx->vidx.part][idx->vidx.ring].insert( idx->vidx.vertex + 1, SnappedToRefNode );
              subjSnapIndex.reset( new QgsSnapIndex( center, 10 * snapTolerance ) );
              subjSnapIndex->addGeometry( subjGeom );
            }
          }
        }
      }
    }
  }
  subjSnapIndex.reset();
  origSubjSnapIndex.reset();
  origSubjGeom.reset();

  // Pass 3: remove superfluous vertices: all vertices which are snapped to a segment and not preceded or succeeded by an unsnapped vertex
  for ( int iPart = 0, nParts = subjGeom->partCount(); iPart < nParts; ++iPart )
  {
    for ( int iRing = 0, nRings = subjGeom->ringCount( iPart ); iRing < nRings; ++iRing )
    {
      bool ringIsClosed = subjGeom->vertexAt( QgsVertexId( iPart, iRing, 0 ) ) == subjGeom->vertexAt( QgsVertexId( iPart, iRing, subjGeom->vertexCount( iPart, iRing ) - 1 ) );
      for ( int iVert = 0, nVerts = polyLineSize( subjGeom, iPart, iRing ); iVert < nVerts; ++iVert )
      {
        int iPrev = ( iVert - 1 + nVerts ) % nVerts;
        int iNext = ( iVert + 1 ) % nVerts;
        QgsPointV2 pMid = subjGeom->vertexAt( QgsVertexId( iPart, iRing, iVert ) );
        QgsPointV2 pPrev = subjGeom->vertexAt( QgsVertexId( iPart, iRing, iPrev ) );
        QgsPointV2 pNext = subjGeom->vertexAt( QgsVertexId( iPart, iRing, iNext ) );

        if ( subjPointFlags[iPart][iRing][iVert] == SnappedToRefSegment &&
             subjPointFlags[iPart][iRing][iPrev] != Unsnapped &&
             subjPointFlags[iPart][iRing][iNext] != Unsnapped &&
             QgsGeometryUtils::sqrDistance2D( QgsGeometryUtils::projPointOnSegment( pMid, pPrev, pNext ), pMid ) < 1E-12 )
        {
          if ( ( ringIsClosed && nVerts > 3 ) || ( !ringIsClosed && nVerts > 2 ) )
          {
            subjGeom->deleteVertex( QgsVertexId( iPart, iRing, iVert ) );
            subjPointFlags[iPart][iRing].removeAt( iVert );
            iVert -= 1;
            nVerts -= 1;
          }
          else
          {
            // Don't delete vertices if this would result in a degenerate geometry
            break;
          }
        }
      }
    }
  }

  return QgsGeometry( subjGeom );
}

int QgsGeometrySnapper::polyLineSize( const QgsAbstractGeometry *geom, int iPart, int iRing ) const
{
  int nVerts = geom->vertexCount( iPart, iRing );

  if ( dynamic_cast< const QgsSurface * >( geom ) )
  {
    QgsPointV2 front = geom->vertexAt( QgsVertexId( iPart, iRing, 0 ) );
    QgsPointV2 back = geom->vertexAt( QgsVertexId( iPart, iRing, nVerts - 1 ) );
    if ( front == back )
      return nVerts - 1;
  }

  return nVerts;
}
