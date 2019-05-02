#include "sysdep.h"
#include "console.h"
#include "gui.h"
#include "s_files.h"

#include <qwidget.h>
#include <qframe.h>
#include <qlabel.h>
#include <qdialog.h>
#include <qbutton.h>
#include <qpushbt.h>
#include <qfiledlg.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>


#define DEBUGX(x) // printf x

unsigned long HaveGUIDialogs = GUIDLG_FILE | GUIDLG_CHOICE;

int DLGGetFile(GView *v, const char *Prompt, unsigned int BufLen,
	       char *FileName, int Flags) {
    QString fn;
    char filter[MAXPATH] = "*";
    char directory[MAXPATH];

    assert(BufLen > 0);

    JustDirectory(FileName, directory, sizeof(directory));

    DEBUGX(("Doing file dialog\n"));
    if (Flags & GF_SAVEAS) {
        fn = QFileDialog::getSaveFileName(directory, filter);
    } else {
        fn = QFileDialog::getOpenFileName(directory, filter);
    }
    DEBUGX(("File dialog done\n"));
    if (fn.isNull())
        return 0;
    strncpy(FileName, fn, BufLen);
    FileName[BufLen - 1] = 0;
    DEBUGX(("selected %s\n", FileName));
    return FileName[0] ? 1 : 0;
}

int DLGPickChoice(GView * /*v*/, const char * /*ATitle*/, int /*NSel*/, va_list /*ap*/, int /*Flags*/) {
    assert(1==0);
    return 0;
}


int DLGGetStr(GView * /*View*/, const char * /*Prompt*/, unsigned int /*BufLen*/, char * /*Str*/, int /*HistId*/, int /*Flags*/) {
    assert(1 == 0);
    return 0;
}

const int kMaxButton = 16;

class QChoiceBox : public QDialog {
    Q_OBJECT
public:
    QChoiceBox(QWidget *parent=0, const char *name=0);

    void setText(const char *text);
    void addButton(const char *text);

    void adjustSize();

    int getChoice() { return buttonActivated; }

public slots:
    void pressed();
    void released();
    void clicked();

protected:
    void resizeEvent(QResizeEvent *);

private:
    QLabel *label;
    QPushButton *button[kMaxButton];
    int buttonCount;
    int buttonArmed;
    int buttonSelected;
    int buttonActivated;
    void *reserved1;
    void *reserved2;
};


QChoiceBox::QChoiceBox(QWidget *parent, const char *name) :
    QDialog(parent, name, TRUE),
    label(new QLabel(this, "text")),
    buttonCount(0),
    buttonArmed(-1),
    buttonSelected(-1),
    buttonActivated(-1)
{
    //initMetaObject();

    CHECK_PTR(label);
    label->setAlignment(AlignLeft);
    QFont font("Helvetica", 12, QFont::Bold);
    label->setFont(font );
}

void QChoiceBox::setText(const char *text) {
    label->setText(text);
}

void QChoiceBox::addButton(const char *text) {
    assert(buttonCount < kMaxButton);

    button[buttonCount] = new QPushButton(this);
    CHECK_PTR(button[buttonCount]);
    connect(button[buttonCount], SIGNAL(clicked()), SLOT(clicked()));
    connect(button[buttonCount], SIGNAL(pressed()), SLOT(pressed()));
    connect(button[buttonCount], SIGNAL(released()), SLOT(released()));
    button[buttonCount]->setFont(QFont("Helvetica", 12, QFont::Bold));
    button[buttonCount]->setText(text ? text : "?" );
    buttonCount++;
}

void QChoiceBox::adjustSize() {
    int w_buttons = 0;

    for (int i = 0; i < buttonCount; i++) {
        button[i]->adjustSize();
        w_buttons += button[i]->width() + 10;
    }

    label->adjustSize();
        
    QString labelStr = label->text();
    int nlines = labelStr.contains('\n');
    QFontMetrics fm = label->fontMetrics();
    nlines += 2;
    int w = QMAX(w_buttons, label->width());
    int h = button[0]->height() + fm.lineSpacing()*nlines;
    resize( w + w/3, h + h/3 );
}

void QChoiceBox::resizeEvent( QResizeEvent * ) {
    int i;
    
    for (i = 0; i < buttonCount; i++) {
        button[i]->adjustSize();
    }
    label->adjustSize();
    int h = (height() - button[0]->height() - label->height())/3;
    int x = 10;

    for (i = 0; i < buttonCount; i++) {
        button[i]->move(x, height() - h - button[i]->height());
        x += button[i]->width() + 10;
    }
    label->move( 10, h );
}

void QChoiceBox::pressed() {
    int i;

    buttonSelected = -1;
    for (i = 0; i < buttonCount; i++)
        if (button[i]->isDown())
            buttonSelected = i;
    buttonArmed = buttonSelected;
    DEBUGX(("selected: %d\n", buttonSelected));
}

void QChoiceBox::released() {
    buttonSelected = -1;
    DEBUGX(("released\n"));
}

void QChoiceBox::clicked() {
    buttonActivated = buttonArmed;
    DEBUGX(("activated: %d\n", buttonActivated));
    accept();
}

#include "g_qt_dlg.moc"

int DLGPickChoice(GView *v, char *ATitle, int NSel, va_list ap, int Flags) {
    QChoiceBox *cb = new QChoiceBox();
    CHECK_PTR(cb);
    cb->setCaption( ATitle );

    for (int i = 0; i < NSel; i++)
        cb->addButton(va_arg(ap, char *));

    char msg[1024];
    char *fmt;
    fmt = va_arg(ap, char *);
    vsprintf(msg, fmt, ap);
    
    cb->setText(msg);
    
    int retcode = cb->exec();
    
    delete cb;
    if (retcode == QDialog::Accepted)
        return cb->getChoice();
    return -1;
}

int DLGGetFind(GView *View, SearchReplaceOptions &sr) {
    assert(1==0);
    return 0;
}

int DLGGetFindReplace(GView *View, SearchReplaceOptions &sr) {
    assert(1==0);
    return 0;
}

int DLGGetStr(GView *View, char *Prompt, unsigned int BufLen, char *Str, int HistId, int Flags) {
    assert(1 == 0);
    return 0;
}
