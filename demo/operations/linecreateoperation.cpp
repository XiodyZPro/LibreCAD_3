#include "linecreateoperation.h"

#include "cad/primitive/line.h"
#include "operationfinishedevent.h"

LineCreateOperation::LineCreateOperation(QGraphicsView* graphicsView, SnapManagerPtr snapManager) : Operation(), _graphicsView(graphicsView), _snapManager(snapManager) {
    connect(graphicsView, SIGNAL(drawEvent(const DrawEvent&)),
            this, SLOT(on_drawEvent(const DrawEvent&)));
    connect(snapManager.get(), SIGNAL(snapPointEvent(const SnapPointEvent&)),
            this, SLOT(on_SnapPoint_Event(const SnapPointEvent&)));


    _waitForFirstClick = new QSnappedState();
    _waitForSecondClick = new QSnappedState();
    QSnappedState* _finishLine = new QSnappedState();
    QFinalState* finishState = new QFinalState();

    _waitForFirstClick->addTransition(qobject_cast<SnapManager*>(snapManager.get()),  "mouseReleaseEvent(MouseReleaseEvent)", _waitForSecondClick);
    _waitForFirstClick->assignProperty(this, "currentState", 1);

    _waitForSecondClick->addTransition(qobject_cast<SnapManager*>(snapManager.get()),  "mouseReleaseEvent(MouseReleaseEvent)", _finishLine);
    _waitForSecondClick->assignProperty(this, "hasStartPoint", true);
    _waitForSecondClick->assignProperty(this, "currentState", 2);
    _waitForSecondClick->assignSnapPoint(this, "startPoint");

    _finishLine->addTransition(finishState);
    _finishLine->assignProperty(this, "hasEndPoint", true);
    _finishLine->assignSnapPoint(this, "endPoint");
    _finishLine->assignProperty(this, "currentState", 3);

    _machine.addState(_waitForFirstClick);
    _machine.addState(_waitForSecondClick);
    _machine.addState(_finishLine);
    _machine.addState(finishState);
    _machine.setInitialState(_waitForFirstClick);
    _machine.setGlobalRestorePolicy(QStateMachine::RestoreProperties);
    _machine.start();

    QObject::connect(&_machine, SIGNAL(finished()), this, SLOT(lineCreationFinished()));
}

void LineCreateOperation::lineCreationFinished() {
    OperationFinishedEvent of;
    emit operationFinished(of);
}

void LineCreateOperation::restart() {
    _machine.setInitialState(_waitForFirstClick);
    _machine.start();
}

lc::CADEntityPtr LineCreateOperation::cadEntity(const QList<lc::MetaTypePtr>& metaTypes) const {
    return lc::LinePtr(new lc::Line(_startPoint, _endPoint, metaTypes));
}

void LineCreateOperation::on_drawEvent(const DrawEvent& event) {
    bool s = property("hasStartPoint").toBool();
    bool e = property("hasEndPoint").toBool();

    // For drawing we should create a interface to teh drawitems in the viewer so we always
    // draw something exactly like how the final drawing classes will draw entities
    if (s == true && e == false) {
        event.painter()->drawLine(_startPoint.pointF(), _lastSnapEvent.snapPoint().pointF());
    }

    if (s == true && e == true) {
        event.painter()->drawLine(_startPoint.pointF(), _endPoint.pointF());
    }

}

void LineCreateOperation::on_SnapPoint_Event(const SnapPointEvent& event) {
    _lastSnapEvent = event;
}


OperationPtr LineCreateOperation::next() const {
    // Create a new line end set the start point to the end point of the last operation
    LineCreateOperation* lco = new LineCreateOperation(this->_graphicsView, this->_snapManager);
    lco->_machine.setInitialState(lco->_waitForSecondClick);
    lco->_machine.start();
    lco->_startPoint = this->_endPoint;
    return OperationPtr(lco);
}