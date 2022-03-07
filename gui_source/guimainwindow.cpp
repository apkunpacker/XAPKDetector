/* Copyright (c) 2020-2022 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "guimainwindow.h"
#include "ui_guimainwindow.h"

GuiMainWindow::GuiMainWindow(QWidget *pParent) :
    QMainWindow(pParent),
    ui(new Ui::GuiMainWindow)
{
    ui->setupUi(this);

    setWindowTitle(XOptions::getTitle(X_APPLICATIONDISPLAYNAME,X_APPLICATIONVERSION));

    setAcceptDrops(true);

    g_fwOptions={};

//    ui->pushButtonClassesDex->setEnabled(false);
    ui->pushButtonSignature->setEnabled(false);

    g_xOptions.setName(X_OPTIONSFILE);

    g_xOptions.addID(XOptions::ID_VIEW_STYLE,"Fusion");
    g_xOptions.addID(XOptions::ID_VIEW_QSS,"");
    g_xOptions.addID(XOptions::ID_VIEW_LANG,"System");
    g_xOptions.addID(XOptions::ID_VIEW_STAYONTOP,false);
    g_xOptions.addID(XOptions::ID_FILE_SAVELASTDIRECTORY,true);
    g_xOptions.addID(XOptions::ID_FILE_SAVEBACKUP,true);

#ifdef Q_OS_WIN
    g_xOptions.addID(XOptions::ID_FILE_CONTEXT,"*");
#endif

    StaticScanOptionsWidget::setDefaultValues(&g_xOptions);
    SearchSignaturesOptionsWidget::setDefaultValues(&g_xOptions);
    XHexViewOptionsWidget::setDefaultValues(&g_xOptions);
    XDisasmViewOptionsWidget::setDefaultValues(&g_xOptions);

    g_xOptions.load();

    g_xShortcuts.setName(X_SHORTCUTSFILE);

    g_xShortcuts.addGroup(XShortcuts::GROUPID_STRINGS);
    g_xShortcuts.addGroup(XShortcuts::GROUPID_SIGNATURES);
    g_xShortcuts.addGroup(XShortcuts::GROUPID_HEX);
    g_xShortcuts.addGroup(XShortcuts::GROUPID_DISASM);
    g_xShortcuts.addGroup(XShortcuts::GROUPID_ARCHIVE);

    g_xShortcuts.load();

    ui->widgetArchive->setGlobal(&g_xShortcuts,&g_xOptions);

    adjustWindow();

    ui->widgetArchive->setGlobal(&g_xShortcuts,&g_xOptions);

    if(QCoreApplication::arguments().count()>1)
    {
        handleFile(QCoreApplication::arguments().at(1));
    }
}

GuiMainWindow::~GuiMainWindow()
{
    g_xOptions.save();
    g_xShortcuts.save();

    delete ui;
}

void GuiMainWindow::handleFile(QString sFileName)
{
    QFileInfo fi(sFileName);

    if(fi.isFile())
    {
        ui->lineEditFileName->setText(sFileName);
        
        ui->widgetArchive->setFileName(sFileName,g_fwOptions,QSet<XBinary::FT>(),this);

//        ui->pushButtonClassesDex->setEnabled(XArchives::isArchiveRecordPresent(sFileName,"classes.dex"));
        ui->pushButtonSignature->setEnabled(XFormats::isSigned(sFileName));

        if(g_xOptions.isScanAfterOpen())
        {
            scanFile(sFileName);
        }

        g_xOptions.setLastDirectory(sFileName);
    }
}

void GuiMainWindow::on_pushButtonExit_clicked()
{
    this->close();
}

void GuiMainWindow::on_pushButtonOpenFile_clicked()
{
    QString sDirectory=g_xOptions.getLastDirectory();

    QString sFileName=QFileDialog::getOpenFileName(this,tr("Open file")+QString("..."),sDirectory,tr("All files")+QString(" (*)"));

    if(!sFileName.isEmpty())
    { 
        handleFile(sFileName);
    }
}

void GuiMainWindow::on_pushButtonScan_clicked()
{
    QString sFileName=ui->lineEditFileName->text().trimmed();

    if(sFileName!="")
    {
        scanFile(sFileName);
    }
}

void GuiMainWindow::on_pushButtonAbout_clicked()
{
    DialogAbout di(this);

    di.exec();
}

void GuiMainWindow::on_pushButtonShortcuts_clicked()
{
    DialogShortcuts dialogShortcuts(this);

    dialogShortcuts.setData(&g_xShortcuts);

    dialogShortcuts.exec();

    adjustWindow();
}

void GuiMainWindow::dragEnterEvent(QDragEnterEvent *pEvent)
{
    pEvent->acceptProposedAction();
}

void GuiMainWindow::dragMoveEvent(QDragMoveEvent *pEvent)
{
    pEvent->acceptProposedAction();
}

void GuiMainWindow::dropEvent(QDropEvent *pEvent)
{
    const QMimeData *pMimeData=pEvent->mimeData();

    if(pMimeData->hasUrls())
    {
        QList<QUrl> urlList=pMimeData->urls();

        if(urlList.count())
        {
            QString sFileName=urlList.at(0).toLocalFile();

            sFileName=XBinary::convertFileName(sFileName);

            handleFile(sFileName);
        }
    }
}

void GuiMainWindow::on_pushButtonOptions_clicked()
{
    DialogOptions dialogOptions(this,&g_xOptions);

    dialogOptions.exec();

    adjustWindow();
}

void GuiMainWindow::adjustWindow()
{
//    ui->widgetViewer->adjustView();

    g_xOptions.adjustStayOnTop(this);
}

void GuiMainWindow::on_pushButtonHex_clicked()
{
    QString sFileName=ui->lineEditFileName->text().trimmed();

    if(sFileName!="")
    {
        QFile file;
        file.setFileName(sFileName);

        if(XBinary::tryToOpen(&file))
        {
            XHexView::OPTIONS options={};

            DialogHexView dialogHex(this,&file,options);
            dialogHex.setGlobal(&g_xShortcuts,&g_xOptions);

            dialogHex.exec();

            file.close();
        }
    }
}

void GuiMainWindow::on_pushButtonStrings_clicked()
{
    QString sFileName=ui->lineEditFileName->text().trimmed();

    if(sFileName!="")
    {
        QFile file;
        file.setFileName(sFileName);

        if(file.open(QIODevice::ReadOnly))
        {
            SearchStringsWidget::OPTIONS options={};
            options.bAnsi=true;
            options.bUnicode=true;

            DialogSearchStrings dialogSearchStrings(this);
            dialogSearchStrings.setData(&file,options,true);
            dialogSearchStrings.setGlobal(&g_xShortcuts,&g_xOptions);

            dialogSearchStrings.exec();

            file.close();
        }
    }
}

void GuiMainWindow::on_pushButtonHash_clicked()
{
    QString sFileName=ui->lineEditFileName->text().trimmed();

    if(sFileName!="")
    {
        QFile file;
        file.setFileName(sFileName);

        if(file.open(QIODevice::ReadOnly))
        {
            DialogHash dialogHash(this);
            dialogHash.setData(&file,XBinary::FT_UNKNOWN);
            dialogHash.setGlobal(&g_xShortcuts,&g_xOptions);

            dialogHash.exec();

            file.close();
        }
    }
}

void GuiMainWindow::on_pushButtonEntropy_clicked()
{
    QString sFileName=ui->lineEditFileName->text().trimmed();

    if(sFileName!="")
    {
        QFile file;
        file.setFileName(sFileName);

        if(file.open(QIODevice::ReadOnly))
        {
            DialogEntropy dialogEntropy(this);
            dialogEntropy.setData(&file);
            dialogEntropy.setGlobal(&g_xShortcuts,&g_xOptions);

            dialogEntropy.exec();

            file.close();
        }
    }
}

void GuiMainWindow::scanFile(QString sFileName)
{
    QFile file;
    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadOnly))
    {
        DialogStaticScan dialogStaticScan(this);
        dialogStaticScan.setData(&file,true);
        //dialogStaticScan.setShortcuts(&g_xShortcuts);

        dialogStaticScan.exec();

        file.close();
    }
}

void GuiMainWindow::on_pushButtonClassesDex_clicked()
{
    QString sFileName=ui->lineEditFileName->text().trimmed();

    if(sFileName!="")
    {
        QTemporaryFile fileTemp;

        if(fileTemp.open())
        {
            QString sTempFileName=fileTemp.fileName();

            if(XArchives::decompressToFile(sFileName,"classes.dex",sTempFileName))
            {
                QFile file;
                file.setFileName(sTempFileName);

                if(file.open(QIODevice::ReadOnly))
                {
                    g_fwOptions.nStartType=SDEX::TYPE_HEADER;
                    g_fwOptions.sTitle="classes.dex";

                    DialogDEX dialogDEX(this);
                    dialogDEX.setGlobal(&g_xShortcuts,&g_xOptions);
                    dialogDEX.setData(&file,g_fwOptions);

                    dialogDEX.exec();

                    file.close();
                }
            }
        }
    }
}

void GuiMainWindow::on_pushButtonSignature_clicked()
{
    QString sFileName=ui->lineEditFileName->text().trimmed();

    if(sFileName!="")
    {
        XBinary::OFFSETSIZE os=XFormats::getSignOffsetSize(sFileName);

        if(os.nSize)
        {
            QFile file;
            file.setFileName(sFileName);

            if(XBinary::tryToOpen(&file))
            {
                SubDevice sd(&file,os.nOffset,os.nSize);

                if(XBinary::tryToOpen(&sd))
                {
                    XHexView::OPTIONS options={};
                    options.sTitle=tr("Signature");
                    options.nStartAddress=os.nOffset;

                    DialogHexView dialogHex(this,&sd,options);
                    dialogHex.setGlobal(&g_xShortcuts,&g_xOptions);

                    dialogHex.exec();

                    sd.close();
                }

                file.close();
            }
        }
    }
}

void GuiMainWindow::on_pushButtonDEX_clicked()
{

}

void GuiMainWindow::on_pushButtonELF_clicked()
{

}
