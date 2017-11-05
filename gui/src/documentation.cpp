#include <iostream>

#include <QLabel>
#include <QLineEdit>
#include <QCompleter>
#include <QTextBrowser>
#include <QVBoxLayout>

#include "gui/documentation.hpp"

void Documentation::insert(QString module, QString name, QString doc)
{
    docs[module][name].doc = doc;
}

////////////////////////////////////////////////////////////////////////////////

QScopedPointer<Documentation> DocumentationPane::docs;

DocumentationPane::DocumentationPane()
{
    Q_ASSERT(docs.data());

    // Flatten documentation into a single-level map
    QMap<QString, QString> fs;
    QMap<QString, QString> tags;
    QMap<QString, QString> mods;
    for (auto mod : docs->docs.keys())
    {
        for (auto f : docs->docs[mod].keys())
        {
            fs[f] = docs->docs[mod][f].doc;
            tags.insert(f, "i" + QString::fromStdString(
                        std::to_string(tags.size())));
            mods.insert(f, mod);
        }
    }

    // Unpack documentation into a text box
    auto txt = new QTextBrowser();
    for (auto f : fs.keys())
    {
        const auto doc = fs[f];

        auto f_ = doc.count(" ") ? doc.split(" ")[0] : "";
        txt->insertHtml(
                "<tt><a name=\"" + tags[f] +
                "\" href=\"#" + tags[f] + "\">" + f + "</a>" +
                "&nbsp;&nbsp;&nbsp;&nbsp;" +
                "<font color=\"silver\">" + mods[f] + "</font>" +
                "</tt><br>");
        if (f_ != f)
        {
            txt->insertHtml("<tt>" + f + "</tt>");
            txt->insertHtml(": alias for ");
            if (fs.count(f_) != 1)
            {
                std::cerr << "DocumentationPane: missing alias "
                          << f_.toStdString() << " for " << f.toStdString()
                          << std::endl;
                txt->insertHtml("<tt>" + f_ + "</tt> (missing)\n");
            }
            else
            {
                txt->insertHtml("<tt><a href=\"#" + tags[f_] + "\">" + f_ + "</a></tt><br>");
            }
            txt->insertPlainText("\n");
        }
        else
        {
            auto lines = doc.split("\n");
            if (lines.size() > 0)
            {
                txt->insertHtml("<tt>" + lines[0] + "</tt><br>");
            }
            for (int i=1; i < lines.size(); ++i)
            {
                txt->insertPlainText(lines[i] + "\n");
            }
            txt->insertPlainText("\n");
        }
    }
    {   // Erase the two trailing newlines
        auto cursor = QTextCursor(txt->document());
        cursor.movePosition(QTextCursor::End);
        cursor.deletePreviousChar();
        cursor.deletePreviousChar();
    }
    txt->setReadOnly(true);
    txt->scrollToAnchor("#i1");

    {
        int max_width = 0;
        QFontMetrics fm(txt->font());
        for (auto line : txt->toPlainText().split("\n"))
        {
            max_width = std::max(max_width, fm.width(line));
        }
        txt->setMinimumWidth(max_width + 40);
    }

    // Build a search bar
    auto edit = new QLineEdit;
    auto completer = new QCompleter(fs.keys());
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    edit->setCompleter(completer);
    connect(completer, QOverload<const QString&>::of(&QCompleter::highlighted),
            txt, [=](const QString& str){
                if (tags.count(str))
                {
                    txt->scrollToAnchor(tags[str]);
                }
            });
    connect(edit, &QLineEdit::textChanged, txt, [=](const QString& str){
                for (auto& t : tags.keys())
                {
                    if (t.startsWith(str))
                    {
                        txt->scrollToAnchor(tags[t]);
                        return;
                    }
                }
            });

    auto layout = new QVBoxLayout();
    auto row = new QHBoxLayout;
    layout->addWidget(txt);
    row->addWidget(new QLabel("🔍  "));
    row->addWidget(edit);
    layout->addItem(row);
    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);

    setWindowTitle("Shape reference");
    setWindowFlags(Qt::Tool | Qt::CustomizeWindowHint |
                   Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_DeleteOnClose);

    show();
    edit->setFocus();
}

void DocumentationPane::setDocs(Documentation* ds)
{
    docs.reset(ds);
}

bool DocumentationPane::hasDocs()
{
    return docs.data();
}