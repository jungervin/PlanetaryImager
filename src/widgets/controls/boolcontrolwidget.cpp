/*
 * Copyright (C) 2016  Marco Gulino <marco@gulinux.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "boolcontrolwidget.h"
#include <QCheckBox>

struct BoolControlWidget::Private {
  QCheckBox *edit;
};

BoolControlWidget::BoolControlWidget(QWidget* parent): ControlWidget(parent), dptr()
{
  layout()->addWidget(d->edit = new QCheckBox);
  connect(d->edit, &QCheckBox::toggled, [=](bool checked) { emit valueChanged({checked}); });
}

BoolControlWidget::~BoolControlWidget()
{

}

void BoolControlWidget::update(const Imager::Control& setting)
{
  d->edit->setChecked(setting.value == 1);
}

QVariant BoolControlWidget::value() const
{
  return {d->edit->isChecked()};
}
