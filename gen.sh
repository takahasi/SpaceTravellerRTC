#!/bin/sh

rtc-template -bcxx \
    --module-name=SpaceTraveller --module-type='DataFlowComponent' \
    --module-desc='Input component for SpaceTraveller of 3D Connexion ' \
    --module-version=1.0 --module-vendor='Takahashi' \
    --module-category=example \
    --module-comp-type=DataFlowComponent --module-act-type=SPORADIC \
    --module-max-inst=10 --outport=out:TimedDoubleSeq
