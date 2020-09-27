#include <iostream>
#include <fstream>
#include <sstream>
#include <QPainter>
#include <QFileDialog>
#include <QMouseEvent>
#include <chrono>
#include "GraphicWidget.h"

GraphicWidget::GraphicWidget(QWidget* parent) : QWidget(parent)
{
    fileName = "No file";
    selectedResidue = -1;
    rectWidth = 18;
    rectHeight = 20;
    displayShortcuts = true;
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool GraphicWidget::readPdbFile(void)
{
    // function duration - start
    auto start = std::chrono::high_resolution_clock::now();

    std::cout << std::endl << "Reading PDB file..." << std::endl;

    std::ifstream ifile;

    ifile.open( fileName.toLatin1().constData() );
    if( ifile.fail() ){
        std::cout << "ERROR: unable to open input file!" << std::endl;
        return(false);
    }

    std::string line;
    std::string recordName;
    int numLine = 1;

    while ( !ifile.eof() ){
        getline(ifile,line);
        if ( line.length() >= 6 ){
            recordName = line.substr(0,6);
            if ( recordName == "ATOM  " ||
                 recordName == "HETATM") {
                Atom atom;
                atom.readLine(line,numLine);
                atoms.push_back(atom);
            } else {
                std::cout << "Warning: Line (" << numLine << ") does not contain either ATOM or HETATM record" << std::endl;
            }
            numLine++;
        }
    }

    ifile.close();

    // print atoms
    int selection = atoms.size();

    for(int i=0; i < selection; i++) {
        atoms[i].print();
    }

    // function duration - stop & results
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "Time taken by function: "
             << duration.count() << " microseconds" << std::endl;

    return(true);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

bool GraphicWidget::setResidues(void)
{
    int firstAtom = 0;                  // index of the first atom in a residue
    int selection = atoms.size();     // how many atoms are selected

    for(int i=0; i < selection; i++) {
        if ( (i+1) == selection ){ // last atom in molecule
            if ( atoms[i].getResidueNumber() != atoms[i-1].getResidueNumber() ){  // distinguish between two different residue

                Residue residue;

                residue.setResidueNumber(atoms[i].getResidueNumber());
                residue.setResidueName(atoms[i].getResidueName());

                residue.setAtomLast(i);
                residue.setAtomFirst(i);

                residues.push_back(residue);
            }
        } else { // all other atoms
            if ( atoms[i].getResidueNumber() != atoms[i+1].getResidueNumber() ){  // distinguish between two different residue

                Residue residue;

                residue.setResidueNumber(atoms[i].getResidueNumber());
                residue.setResidueName(atoms[i].getResidueName());

                residue.setAtomLast(i);
                residue.setAtomFirst(firstAtom);
                firstAtom = i+1;

                residues.push_back(residue);
            }
        }
    }

    // print residues
    selection = residues.size();

    for(int i=0; i < selection; i++) {
        residues[i].print();
    }

    return(true);
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

// paint & mouse events

void GraphicWidget::paintEvent(QPaintEvent* p_event)
{
    QPainter painter(this);
    QPen pen1(Qt::black, 4);
    QPen pen2(Qt::red, 3);
    QFont font1("Helvetica",9,0,false);

    // file name
    painter.setPen(pen1);
    painter.setFont(font1);
    painter.drawText(10,10,"File:");
    painter.drawText(10,30,fileName);

    // print symbol of residues
    painter.setFont(font1);
    int selection = residues.size();
    int column = 0;
    int xPos = 10;  // start position of the first residue
    int yPos = 56;  // start position of the first residue
    int r = 0;
    int g = 0;
    int b = 0;
    for(int i=0; i < selection; i++) {
        // draw rectangle
        std::tie(r, g, b) = residues[i].getColorRgb();
        painter.setBrush(QColor(r,g,b));
        painter.setPen(Qt::NoPen);
        painter.drawRect(xPos,yPos,rectWidth,rectHeight);
        residues[i].setPosXY(xPos,yPos);
        // print symbol
        if ( displayShortcuts == true ){
            painter.setPen(pen1);
            painter.drawText(xPos+(rectWidth*0.28),yPos+(rectHeight*0.78),QChar(residues[i].getResidueChar()));
        }
        // check column
        if ( column == 19 ){
            yPos = yPos + rectHeight + 8;   // 8 = space between rows
            xPos = xPos - (19*rectWidth);
            column = 0;
        } else {
            // set x position
            xPos = xPos + rectWidth;
            column++;
        }
    }

    // add red rect & desc for selected residue
    if( ( selection > 0 ) && ( selectedResidue > -1 ) ){
        painter.setBrush(Qt::NoBrush);
        painter.setPen(pen2);
        painter.drawRect(residues[selectedResidue].getPosX(),residues[selectedResidue].getPosY(),rectWidth,rectHeight);

        std::string residueDescription;
        std::ostringstream ss;
        ss << "Residue: ";
        ss << residues[selectedResidue].getResidueName();
        ss << residues[selectedResidue].getResidueNumber();
        ss << "     Number of atoms: ";
        ss << residues[selectedResidue].getAtomsCount();
        residueDescription = ss.str();

        painter.setPen(pen1);
        painter.drawText(10,height()-10,residueDescription.c_str());
    }


}

void GraphicWidget::mousePressEvent(QMouseEvent* p_event)
{
    int selection = residues.size();

    for(int i=0; i < selection; i++) {
        if ( ( p_event->x() > residues[i].getPosX() && p_event->x() < ( residues[i].getPosX() + rectWidth ) ) && ( p_event->y() > residues[i].getPosY() && p_event->y() < ( residues[i].getPosY() + rectHeight ) ) ){
            selectedResidue = i;
        }
    }

    update();
}

//==============================================================================
//------------------------------------------------------------------------------
//==============================================================================

// buttons

void GraphicWidget::hideGraphic(void)
{
    displayShortcuts = false;
    update();
}

void GraphicWidget::showGraphic(void)
{
    displayShortcuts = true;
    update();
}

void GraphicWidget::openFile(void)
{
    // open PDB file
    QString lastFileName = fileName;
    fileName = QFileDialog::getOpenFileName(this,"Open",".");
    if( fileName.isEmpty() ){
	fileName = lastFileName;
	return;
    }

    std::string strFileName;
    strFileName = fileName.toLatin1().constData();
    std::cout << "File name: " << strFileName << std::endl;

    // clear all previous data
    residues.clear();
    atoms.clear();
    selectedResidue = -1;

    // read PDB file
    readPdbFile();

    // set residues
    setResidues();

    update();
}
