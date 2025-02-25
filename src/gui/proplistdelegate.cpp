#include "proplistdelegate.h"

#include <QStyleOptionComboBox>
#include <QComboBox>
#include <QModelIndex>
#include <QPainter>
#include <QProgressBar>
#include <QApplication>
#include "utilities/utils.h"
#include "torrentcontentfiltermodel.h"
#include "ui_utils/getstyleoptionprogressbar.h"

#include <algorithm>

PropListDelegate::PropListDelegate(QObject* parent /*= 0*/) : QStyledItemDelegate(parent)
{
}

PropListDelegate::~PropListDelegate() = default;

void PropListDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();
    switch (index.column())
    {
    case TorrentContentModelItem::COL_SIZE:
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        opt.text = utilities::SizeToString(index.data().toLongLong());
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

        break;
    }
    case TorrentContentModelItem::COL_PROGRESS:
    {
        if (index.data().toDouble() >= 0)
        {
            QStyleOptionViewItem opt(option);
            initStyleOption(&opt, index);
            QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

            auto progressBarOption = ui_utils::getStyleOptionProgressBar();

            progressBarOption.rect = option.rect.adjusted(2, 6, -2, -7);

            const qreal progress = index.data().toDouble() * 100.;

            progressBarOption.progress = std::min(100, static_cast<int>(progress));
            progressBarOption.text = utilities::ProgressString(progress);
            QApplication::style()->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);
        }
        else
        {
            QStyleOptionViewItem opt(option);
            initStyleOption(&opt, index);
            QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
        }
        break;
    }
    case TorrentContentModelItem::COL_PRIO:
    {
        QString text = "";
        const int priority = index.data().toInt();
        switch (priority)
        {
        case prio::PARTIAL:
            text = tr("Mixed", "Mixed (priorities");
            break;
        case prio::IGNORED:
            text = tr("Skip");
            break;
        case prio::LOW:
            text = tr("Low", "Low (priority)");
            break;
        case prio::HIGH:
            text = tr("High", "High (priority)");
            break;
        default:
            if (priority > prio::LOW && priority < prio::HIGH)
            {
                text = tr("Normal", "Normal (priority)");
            }
            break;
        }
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        opt.text = text;
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
        break;
    }
    default:
        QStyledItemDelegate::paint(painter, option, index);
        break;
    }
    painter->restore();
}

void PropListDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* combobox = static_cast<QComboBox*>(editor);
    // Set combobox index
    const int priority = index.data().toInt();
    switch (priority)
    {
    case prio::PARTIAL:
        combobox->setCurrentIndex(4);
        break;
    case prio::IGNORED:
        combobox->setCurrentIndex(3);    // SKIP
        break;
    case prio::LOW:
        combobox->setCurrentIndex(2);    // LOW
        break;
    case prio::HIGH:
        combobox->setCurrentIndex(0);    // HIGH
        break;
    default:
        if (priority > prio::LOW && priority < prio::HIGH)
        {
            combobox->setCurrentIndex(1);    // treat as NORMAL
            break;
        }
        Q_ASSERT(false);
    }
}

QWidget* PropListDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& /* option */, const QModelIndex& index) const
{
    if (index.column() != TorrentContentModelItem::COL_PRIO) { return 0; }

    auto* editor = new QComboBox(parent);
    editor->setFocusPolicy(Qt::WheelFocus);
    editor->addItem(tr("High", "High (priority)"));
    editor->addItem(tr("Normal", "Normal (priority)"));
    editor->addItem(tr("Low", "Low (priority)"));
    editor->addItem(tr("Skip", "Skip (priority)"));

    const TorrentContentModelItem* tcmi = dynamic_cast<const TorrentContentFilterModel*>(index.model())->getTorrentContentModelItem(index);
    if (tcmi && tcmi->isFolder())
    {
        editor->addItem(tr("Mixed", "Mixed (priority)"));
    }

    VERIFY(connect(editor, SIGNAL(currentIndexChanged(int)), parent, SLOT(setFocus())));

    return editor;
}

void PropListDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QModelIndexList selectedRows(dynamic_cast<const TorrentContentFilterModel*>(index.model())->selectedRows());
    if (selectedRows.empty())
        selectedRows = { index };

    auto* combobox = static_cast<QComboBox*>(editor);
    int value = combobox->currentIndex();
    qDebug("PropListDelegate: setModelData(%d)", value);
    for (const auto& index : qAsConst(selectedRows))
    {
        switch (value)
        {
        case 0:
            model->setData(index, prio::HIGH);
            break;
        case 1:
            model->setData(index, prio::NORMAL);
            break;
        case 2:
            model->setData(index, prio::LOW);
            break;
        case 3:
            model->setData(index, prio::IGNORED);
            break;
        default:
            break;
        }
    }
    emit filteredFilesChanged();
}

void PropListDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const
{
    editor->setGeometry(option.rect);
}
