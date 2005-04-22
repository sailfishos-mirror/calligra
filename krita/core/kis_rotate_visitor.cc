/*
 *  Copyright (c) 2004 Michael Thaler <michael.thaler@physik.tu-muenchen.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <math.h>
#include <qapplication.h>
#include <qwmatrix.h>
#include <qrect.h>

#include <kdebug.h>
#include <klocale.h>

#include "kis_paint_device.h"
#include "kis_rotate_visitor.h"
#include "kis_progress_display_interface.h"
#include "kis_iterators_pixel.h"
#include "kis_selection.h"
#include "kis_painter.h"

void KisRotateVisitor::rotate(double angle, KisProgressDisplayInterface *m_progress) 
{

        const double pi=3.1415926535897932385;
        kdDebug() << "Rotating Code called! Going to rotate image by (angle): " << angle << "\n";

        if(angle>=315 && angle <360){
                angle=angle-360;
        } else if(angle > -360 && angle < -45){
                angle=angle+360;
        }

        //progress info
        m_cancelRequested = false;
        m_progress -> setSubject(this, true, true);
        Q_INT32 progressCurrent=0;
        QRect r = m_dev -> extent();
        emit notifyProgressStage(this,i18n("Rotating Image..."),0);
        
        if (angle>=-45 && angle <45) {
        
                Q_INT32 origHeight = r.height();
                Q_INT32 origWidth = r.width();
                double theta = angle*pi/180;
                double shearX = tan(theta/2);
                double shearY = sin(theta);
                Q_INT32 newWidth = (int) (origWidth + QABS(origHeight * shearX));
                Q_INT32 newHeight = (int) (origHeight + QABS(newWidth * shearY));
                Q_INT32 progressTotal = (int) origHeight + newWidth + newHeight;
                //first perform a shear along the x-axis by tan(theta/2)
                progressCurrent=xShearImage(shearX, m_progress, progressTotal, progressCurrent);
                //next perform a shear along the y-axis by sin(theta)
                progressCurrent=yShearImage(shearY, m_progress, progressTotal, progressCurrent);
                //again perform a shear along the x-axis by tan(theta/2)
                progressCurrent=xShearImage(shearX, m_progress, progressTotal, progressCurrent);
                double deltaX=(origWidth+origHeight*QABS(shearX))*QABS(shearX*shearY);
       
                if (deltaX != 0)
                        xCropImage(deltaX);
                double deltaY=origHeight*QABS(shearX*shearY);
                if (deltaY != 0)
                        yCropImage(deltaY);
        }
        else if (angle>=45 && angle < 135 && angle != 90) {
        
                rotateRight90(m_dev, m_dev, m_dev -> exactBounds());
                Q_INT32 origHeight = r.height();
                Q_INT32 origWidth = r.width();
                double theta=(angle-90)*pi/180;
                double shearX=tan(theta/2);
                double shearY=sin(theta);
                Q_INT32 newWidth = (int)(origWidth + QABS(origHeight * shearX));
                Q_INT32 newHeight = (int)(origHeight + QABS(newWidth * shearY));
                Q_INT32 progressTotal = (int)(origHeight + newWidth + newHeight);
                //first perform a shear along the x-axis by tan(theta/2)
                progressCurrent=xShearImage(shearX, m_progress, progressTotal, progressCurrent);
                //next perform a shear along the y-axis by sin(theta)
                progressCurrent=yShearImage(shearY, m_progress, progressTotal, progressCurrent);
                //again perform a shear along the x-axis by tan(theta/2)
                progressCurrent=xShearImage(shearX, m_progress, progressTotal, progressCurrent);
                double deltaX=(origWidth+origHeight*QABS(shearX))*QABS(shearX*shearY);
                if (deltaX != 0)
                        xCropImage(deltaX);
                double deltaY=origHeight*QABS(shearX*shearY);
                if (deltaY != 0)
                        yCropImage(deltaY);
                        
        }
        else if (angle>=135 && angle < 225 && angle != 180) {
                
                rotate180(m_dev, m_dev, m_dev -> exactBounds());
                Q_INT32 origHeight = r.height();
                Q_INT32 origWidth = r.width();
                double theta=(angle-180)*pi/180;
                double shearX=tan(theta/2);
                double shearY=sin(theta);
                Q_INT32 newWidth = (int)(origWidth + QABS(origHeight * shearX));
                Q_INT32 newHeight = (int)(origHeight + QABS(newWidth * shearY));
                Q_INT32 progressTotal = (int)(origHeight + newWidth + newHeight);
                //first perform a shear along the x-axis by tan(theta/2)
                progressCurrent=xShearImage(shearX, m_progress, progressTotal, progressCurrent);
                //next perform a shear along the y-axis by sin(theta)
                progressCurrent=yShearImage(shearY, m_progress, progressTotal, progressCurrent);
                //again perform a shear along the x-axis by tan(theta/2)
                progressCurrent=xShearImage(shearX, m_progress, progressTotal, progressCurrent);
                double deltaX=(origWidth+origHeight*QABS(shearX))*QABS(shearX*shearY);
                if (deltaX != 0)
                        xCropImage(deltaX);
                double deltaY=origHeight*QABS(shearX*shearY);
                if (deltaY != 0)
                        yCropImage(deltaY);
        } else if(angle>=225 && angle < 315 && angle != 270) {
                
                rotateLeft90(m_dev, m_dev, m_dev -> exactBounds());
                Q_INT32 origHeight = r.height();
                Q_INT32 origWidth = r.width();
                double theta=(angle-270)*pi/180;
                double shearX=tan(theta/2);
                double shearY=sin(theta);
                Q_INT32 newWidth = (int)(origWidth + QABS(origHeight * shearX));
                Q_INT32 newHeight = (int)(origHeight + QABS(newWidth * shearY));
                Q_INT32 progressTotal = (int)(origHeight + newWidth + newHeight);
                //first perform a shear along the x-axis by tan(theta/2)
                progressCurrent=xShearImage(shearX, m_progress, progressTotal, progressCurrent);
                //next perform a shear along the y-axis by sin(theta)
                progressCurrent=yShearImage(shearY, m_progress, progressTotal, progressCurrent);
                //again perform a shear along the x-axis by tan(theta/2)
                progressCurrent=xShearImage(shearX, m_progress, progressTotal, progressCurrent);
                double deltaX=(origWidth+origHeight*QABS(shearX))*QABS(shearX*shearY);
                if (deltaX != 0)
                        xCropImage(deltaX);
                double deltaY=origHeight*QABS(shearX*shearY);
                if (deltaY != 0)
                        yCropImage(deltaY);

        } else if(angle==90) {
		rotateRight90(m_dev, m_dev, m_dev -> exactBounds());
        } else if (angle==180) {
		rotate180(m_dev, m_dev, m_dev -> exactBounds());
        } else if (angle==270) {
		rotateLeft90(m_dev, m_dev, m_dev -> exactBounds());
        }

        emit notifyProgressDone(this);


}

void KisRotateVisitor::shear(double angleX, double angleY, KisProgressDisplayInterface *m_progress) 
{
        kdDebug() << "Shear Code called! Going to shear image by xAngle " << angleX << " and yAngle " << angleY << "\n";
        const double pi=3.1415926535897932385;
        
        //progress info
        m_cancelRequested = false;
        m_progress -> setSubject(this, true, true);
        Q_INT32 progressCurrent=0;
        emit notifyProgressStage(this,i18n("Rotating Image..."),0);
        
        if (angleX != 0 || angleY != 0){
                Q_INT32 origHeight = m_dev -> image() -> height();
                Q_INT32 origWidth = m_dev -> image() -> width();
                double thetaX=angleX*pi/180;
                double shearX=tan(thetaX);
                double thetaY=angleY*pi/180;
                double shearY=tan(thetaY);
                Q_INT32 newWidth = (int)(origWidth + QABS(origHeight * shearX));
                Q_INT32 progressTotal = origHeight + newWidth;
                //first perform a shear along the x-axis by tan(thetaX)
                progressCurrent=xShearImage(shearX, m_progress, progressTotal, progressCurrent);
                //next perform a shear along the y-axis by tan(thetaY)
                progressCurrent=yShearImage(shearY, m_progress, progressTotal, progressCurrent);
                double deltaY=origHeight*QABS(shearX*shearY);
                if (deltaY != 0 && thetaX > 0 && thetaY > 0)
                        yCropImage(deltaY);
                else if (deltaY != 0 && thetaX < 0 && thetaY < 0)
                        yCropImage(deltaY);
        }
}

Q_INT32 KisRotateVisitor::xShearImage(double shearX, KisProgressDisplayInterface * /*m_progress*/, Q_INT32 progressTotal, Q_INT32 progressCurrent)
{
        kdDebug() << "xShearImage called, shear parameter " << shearX << "\n";        
        Q_INT32 width = m_dev -> image() -> width();
        Q_INT32 height = m_dev -> image() -> height();
        double progressStart=(double)progressCurrent / progressTotal * 100; 
         
        //calculate width of the sheared image
        Q_INT32 targetW = (Q_INT32)(width + QABS(height*shearX));
        Q_INT32 targetH = height;

        QUANTUM * newData = new QUANTUM[targetW * targetH * m_dev -> pixelSize()]; // XXX: (BSAR) Not* sizeof(quantum), the pixelsize is the size in bytes already!
	Q_CHECK_PTR(newData);

        QUANTUM *tempRow = new QUANTUM[width * m_dev -> pixelSize()];
	Q_CHECK_PTR(tempRow);

        QUANTUM *pixel = new QUANTUM[m_dev -> pixelSize()];
	Q_CHECK_PTR(pixel);

        QUANTUM *left = new QUANTUM[m_dev -> pixelSize()];
	Q_CHECK_PTR(left);

        QUANTUM *oleft = new QUANTUM[m_dev -> pixelSize()];
	Q_CHECK_PTR(oleft);

        double displacement;
        Q_INT32 displacementInt;
        double weight;
        Q_INT32 currentPos;
        
       
        if(shearX>=0){
                for (Q_INT32 y=0; y < height; y++){
                        emit notifyProgress(this, (int)(y * 100  / progressTotal + progressStart + 1));
                        progressCurrent++;
                        if (m_cancelRequested) {
                                break;
                        }
                        //calculate displacement
                        displacement = (height-y)*shearX;
                        displacementInt = (Q_INT32)(floor(displacement));
                        weight=displacement-displacementInt;
                        //read a row from the image
                        m_dev -> readBytes(tempRow, 0, y, width, 1);
                        //initialize oleft
                        for(int channel = 0; channel < m_dev -> pixelSize(); channel++)
                                oleft[channel]=left[channel]=0;
                        //copy the pixels to the newData array
                        for(Q_INT32 x=0; x < width; x++){
                                currentPos = (y*targetW+x+displacementInt) * m_dev -> pixelSize(); // try to be at least a little efficient
                                for(int channel = 0; channel < m_dev -> pixelSize(); channel++){
                                        pixel[channel]=tempRow[x*m_dev -> pixelSize()+channel];
                                        left[channel]= (Q_INT32)(weight*pixel[channel]);
                                        pixel[channel]=pixel[channel]-left[channel]+oleft[channel];
                                        newData[currentPos  + channel]=pixel[channel];
                                        oleft[channel]=left[channel];
                                }
                        }
                }        
        } else {
                for (Q_INT32 y=0; y < height; y++){
                        emit notifyProgress(this, (int)(y * 100  / progressTotal + progressStart + 1));
                        progressCurrent++;
                        if (m_cancelRequested) {
                                break;
                        }
                        //calculate displacement
                        displacement = y*QABS(shearX);
                        displacementInt = (Q_INT32)(floor(displacement));
                        weight=displacement-displacementInt;
                        //read a row from the image
                        m_dev -> readBytes(tempRow, 0, y, width, 1);
                        //initialize oleft
                        for(int channel = 0; channel < m_dev -> pixelSize(); channel++)
                                oleft[channel]=left[channel]=0;
                        //copy the pixels to the newData array
                        for(Q_INT32 x=0; x < width; x++){
                                currentPos = (y*targetW+x+displacementInt) * m_dev -> pixelSize(); // try to be at least a little efficient
                                for(int channel = 0; channel < m_dev -> pixelSize(); channel++){
                                        pixel[channel]=tempRow[x*m_dev -> pixelSize()+channel];
                                        left[channel]= (Q_INT32)(weight*pixel[channel]);
                                        pixel[channel]=pixel[channel]-left[channel]+oleft[channel];
                                        newData[currentPos  + channel]=pixel[channel];
                                        oleft[channel]=left[channel];
                                }
                        }
                }          
        }        
        //now write newData to the image
        kdDebug() << "write newData to the image!" << "\n";
        m_dev -> writeBytes( newData, 0, 0, targetW, targetH);

        delete[] newData;
	delete[] tempRow;
	delete[] pixel;
	delete[] left;
	delete[] oleft;


        return progressCurrent;
}

Q_INT32 KisRotateVisitor::yShearImage(double shearY, KisProgressDisplayInterface * /*m_progress*/, Q_INT32 progressTotal, Q_INT32 progressCurrent)
{
        kdDebug() << "YShearImage called, shear parameter " << shearY << "\n";
        
        Q_INT32 width = m_dev -> image() -> width();
        Q_INT32 height = m_dev -> image() -> height();
        double progressStart=(double)progressCurrent / progressTotal * 100; 
        
        //calculate widht of the sheared image
        Q_INT32 targetW = width;
        Q_INT32 targetH = (Q_INT32)(height + QABS(width*shearY));

        QUANTUM * newData = new QUANTUM[targetW * targetH * m_dev -> pixelSize()];
        Q_CHECK_PTR(newData);

        //shear the image
        QUANTUM *tempCol = new QUANTUM[height * m_dev -> pixelSize()];
	Q_CHECK_PTR(tempCol);

        QUANTUM *pixel = new QUANTUM[m_dev -> pixelSize()];
	Q_CHECK_PTR(pixel);

        QUANTUM *left = new QUANTUM[m_dev -> pixelSize()];
	Q_CHECK_PTR(left);

        QUANTUM *oleft = new QUANTUM[m_dev -> pixelSize()];
	Q_CHECK_PTR(oleft);

        double displacement;
        Q_INT32 displacementInt;
        double weight;
        Q_INT32 currentPos;

        if(shearY>=0){
                for (Q_INT32 x=0; x < width; x++){
                        emit notifyProgress(this, (int)(x * 100  / progressTotal + progressStart + 1));
                        progressCurrent++;
                        if (m_cancelRequested) {
                                break;
                        }
                        //calculate displacement
                        displacement = x*shearY;
                        displacementInt = (Q_INT32)(floor(displacement));
                        weight=displacement-displacementInt;
                        //read a column from the image
                        m_dev -> readBytes(tempCol, x, 0, 1, height);
                        //initialize oleft
                        for(int channel = 0; channel < m_dev -> pixelSize(); channel++)
                                oleft[channel]=left[channel]=0;
                        //copy the pixels to the newData array
                        for(Q_INT32 y=0; y < height; y++){
                                currentPos = ((y+displacementInt)*targetW+x) * m_dev -> pixelSize(); // try to be at least a little efficient
                                for(int channel = 0; channel < m_dev -> pixelSize(); channel++){
                                        pixel[channel]=tempCol[y*m_dev -> pixelSize()+channel];
                                        left[channel] = (Q_INT32)(weight*pixel[channel]);
                                        pixel[channel]=pixel[channel]-left[channel]+oleft[channel];
                                        newData[currentPos  + channel]=pixel[channel];
                                        oleft[channel]=left[channel];
                                }
                        }
                }
        } else {
                for (Q_INT32 x=0; x < width; x++){
                        emit notifyProgress(this, (int)(x * 100  / progressTotal + progressStart + 1));
                        progressCurrent++;
                        if (m_cancelRequested) {
                                break;
                        }
                        //calculate displacement
                        displacement = (width-x)*QABS(shearY);
                        displacementInt = (Q_INT32)(floor(displacement));
                        weight=displacement-displacementInt;
                        //read a column from the image
                        m_dev -> readBytes(tempCol, x, 0, 1, height);
                        //initialize oleft
                        for(int channel = 0; channel < m_dev -> pixelSize(); channel++)
                                oleft[channel]=left[channel]=0;
                        //copy the pixels to the newData array
                        for(Q_INT32 y=0; y < height; y++){
                                currentPos = ((y+displacementInt)*targetW+x) * m_dev -> pixelSize(); // try to be at least a little efficient
                                for(int channel = 0; channel < m_dev -> pixelSize(); channel++){
                                        pixel[channel]=tempCol[y*m_dev -> pixelSize()+channel];
                                        left[channel] = (Q_INT32)(weight*pixel[channel]);
                                        pixel[channel]=pixel[channel]-left[channel]+oleft[channel];
                                        newData[currentPos  + channel]=pixel[channel];
                                        oleft[channel]=left[channel];
                                }
                        }
                }        
        }
        //now write newData to the image
        kdDebug() << "write newData to the image!" << "\n";
        m_dev -> writeBytes( newData, 0, 0, targetW, targetH);

	delete[] newData;
        delete[] tempCol;
        delete[] pixel;
        delete[] left;
        delete[] oleft;

        return progressCurrent;
}

void KisRotateVisitor::xCropImage(double deltaX)
{
        Q_INT32 width = m_dev -> image() -> width();
        Q_INT32 height = m_dev -> image() -> height();
        //calculate widht of the croped image
        Q_INT32 targetW = (Q_INT32)(width -2 * deltaX + 2);
        Q_INT32 targetH = height;
        QUANTUM * newData = new QUANTUM[targetW * targetH * m_dev -> pixelSize() * sizeof(QUANTUM)];
        QUANTUM *tempRow = new QUANTUM[width * m_dev -> pixelSize() * sizeof(QUANTUM)];
        Q_INT32 currentPos;
        for(Q_INT32 y=0; y < height; y++){
                m_dev -> readBytes(tempRow, 0, y, width, 1);
                for(Q_INT32 x = (Q_INT32)deltaX; x < (Q_INT32)((width - deltaX) + 1); x++){
                        currentPos = (y*targetW+x) * m_dev -> pixelSize();
                        for(int channel = 0; channel < m_dev -> pixelSize(); channel++){
                                newData[currentPos - (int)deltaX*m_dev -> pixelSize() + channel]=tempRow[x*m_dev -> pixelSize()+channel];
                        }    
                }
        }
        kdDebug() << "write newData to the image!" << "\n";
        m_dev -> writeBytes( newData, 0, 0, targetW, targetH);
}

void KisRotateVisitor::yCropImage(double deltaY)
{
        Q_INT32 width = m_dev -> image() -> width();
        Q_INT32 height = m_dev -> image() -> height();
        //calculate width of the croped image
        Q_INT32 targetW = width;
        Q_INT32 targetH = (Q_INT32)(height - 2 * deltaY + 2);
        QUANTUM * newData = new QUANTUM[targetW * targetH * m_dev -> pixelSize() * sizeof(QUANTUM)];
        QUANTUM *tempRow = new QUANTUM[width * m_dev -> pixelSize() * sizeof(QUANTUM)];
        Q_INT32 currentPos;
        for(Q_INT32 y = (Q_INT32)deltaY; y < (Q_INT32)(height - deltaY); y++){
                m_dev -> readBytes(tempRow, 0, y, width, 1);
                for(Q_INT32 x=0; x < width; x++){
                        currentPos = (y*targetW+x) * m_dev -> pixelSize();
                        for(int channel = 0; channel < m_dev -> pixelSize(); channel++){
                                newData[currentPos - (int)deltaY*targetW*m_dev -> pixelSize() + channel]=tempRow[x*m_dev -> pixelSize()+channel];
                        }    
                }
        }
        kdDebug() << "write newData to the image!" << "\n";
        m_dev -> writeBytes( newData, 0, 0, targetW, targetH);
}

bool KisRotateVisitor::rotateRight90(KisPaintDevice *src, KisPaintDevice *dst, QRect r, bool clear)
{
	if (src -> colorStrategy() != dst -> colorStrategy()) return false;

	KisPaintDeviceSP tmp;
	if (src == dst)
		tmp = new KisPaintDevice(src -> colorStrategy(), "temporary");
	else
		tmp = dst;
	

	
	KisStrategyColorSpaceSP cs = src -> colorStrategy();
	
	Q_INT32 y, rx, ry, rw, rh;
	Q_INT32 pixelSize = src -> pixelSize();

	rx = r.x();
	ry = r.y();
	rw = r.width();
	rh = r.height();

	int x = rx;
	
	for (y = rh; y > ry; --y) {
		KisHLineIteratorPixel hit = src -> createHLineIterator(rx, y, rw, true);
		KisVLineIterator vit = tmp -> createVLineIterator(x, 0, rw, true);

		while (!hit.isDone()) {
			
			if (hit.isSelected())  {
 				if (clear)
 					memset(hit.rawData(), 0, pixelSize);

				memcpy(vit.rawData(), hit.oldRawData(), pixelSize);

			}
			++hit;
			++vit;
		}
		++x;
		
		if (m_cancelRequested) break;
		qApp -> processEvents();

	}

	if (src == dst) {

		// Times 2 trick from Kolourpaint
		QPoint oldCenter2 (rx * 2 + rw, ry * 2 + rh);
		QPoint newTopLeft2 (oldCenter2 - QPoint(rh, rw));
	
		int newX = newTopLeft2.x() / 2;
		int newY = newTopLeft2.y() / 2;
	
		KisPainter p(src);
		
		p.bitBlt(newX, newY, COMPOSITE_OVER, tmp, OPACITY_OPAQUE, 0, 0, rh, rw);
		p.end();

	}
	
	return !m_cancelRequested;

}

bool KisRotateVisitor::rotateLeft90(KisPaintDevice *src, KisPaintDevice *dst, QRect r, bool clear)
{
	if (src -> colorStrategy() != dst -> colorStrategy()) return false;

	KisPaintDeviceSP tmp;
	if (src == dst)
		tmp = new KisPaintDevice(src -> colorStrategy(), "temporary");
	else
		tmp = dst;
	
	Q_INT32 y, rx, ry, rw, rh;

	rx = r.x();
	ry = r.y();
	rw = r.width();
	rh = r.height();

	
	Q_INT32 pixelSize = src -> pixelSize();

	int tmpY = 0;
	for (y = ry; y < rh; ++y) {
		// Read the horizontal line from back to front, write onto the vertical column
		KisHLineIteratorPixel hit = src -> createHLineIterator(rx, y, rw, true);
		KisVLineIterator vit = tmp -> createVLineIterator(tmpY, 0, rw, true);

		hit += rw - 1;
		int x = rw;
		while (x > 0) {
			if (hit.isSelected()) {
				memcpy(vit.rawData(), hit.oldRawData(), pixelSize);
				if (clear)
					memset(hit.rawData(), 0, pixelSize);
			}
			--hit;
			++vit;
			--x;
		}
		++tmpY;
		if (m_cancelRequested) break;
		qApp -> processEvents();


	}

	if (src == dst) {

		// Times 2 trick from Kolourpaint
		QPoint oldCenter2 (rx * 2 + rw, ry * 2 + rh);
		QPoint newTopLeft2 (oldCenter2 - QPoint(rh, rw));
	
		int newX = newTopLeft2.x() / 2;
		int newY = newTopLeft2.y() / 2;
	
		KisPainter p(src);
		
		p.bitBlt(newX, newY, COMPOSITE_OVER, tmp, OPACITY_OPAQUE, 0, 0, rh, rw);
		p.end();
		
	}
	return !m_cancelRequested;
}

// Mirror-read the contents of the iterator into tmpLine
void mirrorLine(KisHLineIterator it, Q_UINT8 * tmpLine, Q_INT32 width, Q_INT32 pixelSize)
{
	int le = width * pixelSize - pixelSize;
	
	while (!it.isDone() && le > 0) {
		memcpy(tmpLine + le, it.oldRawData(), pixelSize);
		le -= pixelSize;
		++it;
	}
}

void mirrorLineSelection(KisHLineIteratorPixel it, Q_UINT8 * tmpLine, Q_INT32 width, Q_INT32 pixelSize, bool clear)
{
	int le = width * pixelSize - pixelSize;
	
	while (!it.isDone() && le > 0) {
		if( it.isSelected() ) {
			memcpy(tmpLine + le, it.oldRawData(), pixelSize);
			if (clear)
				memset(it.rawData(), 0, pixelSize); // Clear the original pixel
		}
		else {
			memset(tmpLine + le, 0, pixelSize);
		}
		le -= pixelSize;
		++it;
	}
}

bool KisRotateVisitor::rotate180(KisPaintDevice *src, KisPaintDevice *dst, QRect r, bool clear)
{
	if (src -> colorStrategy() != dst -> colorStrategy())
		return false;

	Q_INT32 pixelSize = src -> pixelSize();
	KisStrategyColorSpaceSP srcCS = src -> colorStrategy();
	
	Q_INT32 x, rx, ry, rw, rh;

	rx = r.x();
	ry = r.y();
	rw = r.width();
	rh = r.height();	

	// We need this tmpLine until we can use decrement iterators
	Q_UINT8 * tmpLine = new Q_UINT8[rw * pixelSize];
	Q_CHECK_PTR(tmpLine);

	// Empty pixel for comparison
	Q_UINT8 * emptyPixel = new Q_UINT8[pixelSize];
	Q_CHECK_PTR(emptyPixel);
	memset(emptyPixel, 0, pixelSize);
	

	if (!src -> hasSelection()) {
		Q_INT32 y1, y2;
		for (y1 = 0, y2 = rh; y1 < rh || y2 > rh; ++y1, --y2) {

			KisHLineIterator itTop = src -> createHLineIterator(rx, y1, rw, false);
			x = 0;
			mirrorLine(itTop, tmpLine, rw, pixelSize);
			dst -> writeBytes(tmpLine, rx, y2, rw, 1);
		
			qApp -> processEvents();
		}
	}
	else {
		Q_UINT8 * mergeLine = new Q_UINT8[(rw * pixelSize)];
		Q_CHECK_PTR(mergeLine);
		Q_INT32 le = rw * pixelSize;
		
		Q_INT32 y1, y2;
		for (y1 = 0, y2 = rh; y1 < rh || y2 > rh; ++y1, --y2) {

			KisHLineIteratorPixel itTop = src -> createHLineIterator(rx, y1, rw, true);
			KisHLineIterator itBottom = dst -> createHLineIterator(rx, y2, rw, true);
			x = 0;
			// This gives a line that's empty except for the selected part.
			mirrorLineSelection(itTop, tmpLine, rw, pixelSize, clear);
			
			// Combine the empty line with the original, unmirrored, unrotated line.
			src -> readBytes(mergeLine, rx, y2, rw, 1);
			
			for (int i = 0; i < le; i += pixelSize) {
				if (memcmp(tmpLine + i, emptyPixel, pixelSize) != 0) {
					memcpy(mergeLine + i, tmpLine + i, pixelSize);
				}
			}

			// Write the finished line with cut-out and all to the dst layer.
			dst -> writeBytes(mergeLine, rx, y2, rw, 1);
			if (m_cancelRequested) break;
			qApp -> processEvents();
		}
		delete [] mergeLine;

	}
	delete [] emptyPixel;
	delete [] tmpLine;

	return true;
}
