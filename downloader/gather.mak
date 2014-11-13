source = $(DEVROOT)\3rdpub\xml

!if [if not exist xml\$(null) mkdir xml]
!endif

all:    xml\tools.h xml\napkin.cpp xml\pool.cpp xml\strout.cpp \
        xml\private.h xml\pubxml.h xml\pubxml.txt xml\xmlapi.htm \
        xml\xmlcont.cpp xml\xmlelem.cpp xml\xmlhelp.cpp \
        xml\xmlparse.cpp xml\xmlwrap.cpp xml\strings.cpp

xml\tools.h  :      $(source)\tools.h
    copy $? $@ > nul

xml\napkin.cpp :    $(source)\napkin.cpp
    copy $? $@ > nul

xml\pool.cpp :      $(source)\pool.cpp
    copy $? $@ > nul

xml\strout.cpp :      $(source)\strout.cpp
    copy $? $@ > nul

xml\strings.cpp :   $(source)\strings.cpp
    copy $? $@ > nul

xml\private.h :     $(source)\private.h
    copy $? $@ > nul

xml\pubxml.h :      $(source)\pubxml.h
    copy $? $@ > nul

xml\pubxml.txt :    $(source)\pubxml.txt
    copy $? $@ > nul

xml\xmlapi.htm :    $(source)\xmlapi.htm
    copy $? $@ > nul

xml\xmlcont.cpp :   $(source)\xmlcont.cpp
    copy $? $@ > nul

xml\xmlelem.cpp :   $(source)\xmlelem.cpp
    copy $? $@ > nul

xml\xmlhelp.cpp :   $(source)\xmlhelp.cpp
    copy $? $@ > nul

xml\xmlparse.cpp :  $(source)\xmlparse.cpp
    copy $? $@ > nul

xml\xmlwrap.cpp :   $(source)\xmlwrap.cpp
    copy $? $@ > nul

clean:
    rm xml\*
