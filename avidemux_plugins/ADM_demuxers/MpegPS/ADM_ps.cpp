/***************************************************************************
    copyright            : (C) 2007/2009 by mean
    email                : fixounet@free.fr
    

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ADM_default.h"
#include "fourcc.h"
#include "DIA_coreToolkit.h"
#include "ADM_indexFile.h"
#include "ADM_ps.h"

#include <math.h>
#define MY_CLASS psHeader
#include "ADM_coreDemuxerMpegTemplate.cpp.h"

uint32_t ADM_UsecFromFps1000(uint32_t fps1000);

/**
      \fn open
      \brief open the flv file, gather infos and build index(es).
*/

uint8_t psHeader::open(const char *name)
{
    char *idxName=(char *)malloc(strlen(name)+6);
    bool r=false;
    FP_TYPE appendType=FP_DONT_APPEND;
    uint32_t append;
    char *type;
    uint64_t startDts;
    uint32_t version=0;

    sprintf(idxName,"%s.idx2",name);
    indexFile index;
    if(!index.open(idxName))
    {
        printf("[psDemux] Cannot open index file %s\n",idxName);
        free(idxName);
        return false;
    }
    if(!index.readSection("System"))
    {
        printf("[psDemux] Cannot read system section\n");
        goto abt;
    }

    version=index.getAsUint32("Version");
    if(version!=ADM_INDEX_FILE_VERSION)
    {
        GUI_Error_HIG(QT_TRANSLATE_NOOP("psdemuxer","Error"), QT_TRANSLATE_NOOP("psdemuxer","This file's index has been created with an older version of avidemux.\nPlease delete the idx2 file and reopen."));
        goto abt;
    }
    type=index.getAsString("Type");
    if(!type || type[0]!='P')
    {
        printf("[psDemux] Incorrect or not found type\n");
        goto abt;
    }
    append=index.getAsUint32("Append");
    printf("[psDemux] Append=%" PRIu32"\n",append);
    if(append) appendType=FP_APPEND;
    if(!parser.open(name,&appendType))
    {
        printf("[psDemux] Cannot open root file %s\n",name);
        goto abt;
    }
    if(!readVideo(&index)) 
    {
        printf("[psDemux] Cannot read Video section of %s\n",idxName);
        goto abt;
    }
    if(!readAudio(&index,name)) 
    {
        printf("[psDemux] Cannot read Audio section of %s => No audio\n",idxName);
    }
    if(!readIndex(&index))
    {
        printf("[psDemux] Cannot read index for file %s\n",idxName);
        goto abt;
    }
    if(readScrReset(&index))
    {
        ADM_info("Adjusting timestamps\n");
        // Update PTS/DTS of video taking SCR Resets into account
        int nbPoints=listOfScrGap.size();
        int index=0;
        uint64_t pivot=listOfScrGap[0].position;
        uint64_t timeOffset=0;
        uint32_t nbImage=ListOfFrames.size();
        for(int i=0;i<nbImage;i++)
        {
            dmxFrame *frame=ListOfFrames[i];
            if(frame->startAt>pivot) // next gap
            {
                    timeOffset=listOfScrGap[index].timeOffset;
                    index++;
                    if(index>=nbPoints) pivot=0xfffffffffffffffLL;
                        else pivot=listOfScrGap[index].position;
            }
            if(frame->dts!=ADM_NO_PTS) frame->dts+=timeOffset;
            if(frame->pts!=ADM_NO_PTS) frame->pts+=timeOffset;
        }
        ADM_info("Adjusted %d scr reset out of %d\n",(int)index,(int)nbPoints);
        ADM_info("Updating audio with list of SCR\n");
        for(int i=0;i<listOfAudioTracks.size();i++)
                  listOfAudioTracks[i]->access->setScrGapList(&listOfScrGap) ;
    }
    updatePtsDts();
    _videostream.dwLength= _mainaviheader.dwTotalFrames=ListOfFrames.size();
    printf("[psDemux] Found %d video frames\n",_videostream.dwLength);
    if(_videostream.dwLength)_isvideopresent=1;
//***********
    
    psPacket=new psPacketLinear(0xE0);
    if(psPacket->open(name,appendType)==false) 
    {
        printf("psDemux] Cannot psPacket open the file\n");
        goto abt;
    }
    r=true;
    for(int i=0;i<listOfAudioTracks.size();i++)
    {
        ADM_psTrackDescriptor *desc=listOfAudioTracks[i];
        ADM_audioStream *audioStream=ADM_audioCreateStream(&desc->header,desc->access);
        if(!audioStream)
        {
            
        }else       
        {
                desc->stream=audioStream;
        }
    }
abt:
    index.close();
    free(idxName);
    printf("[psDemuxer] Loaded %d\n",r);
    return r;
}



/*
    __________________________________________________________
*/

void psHeader::Dump(void)
{
 
}
/**
    \fn close
    \brief cleanup
*/

uint8_t psHeader::close(void)
{
    // Destroy index
    int nb=ListOfFrames.size();
    for(int i=0;i<nb;i++)
    {
        if(ListOfFrames[i]) delete ListOfFrames[i];
        ListOfFrames[i]=0;
    }
    if(psPacket)
    {
        psPacket->close();
        delete psPacket;
        psPacket=NULL;
    }
    nb=listOfAudioTracks.size();
    for(int i=0;i<nb;i++)
    {
        delete listOfAudioTracks[i];
        listOfAudioTracks[i] = 0;
    }
    listOfAudioTracks.clear();
    return 1;
}
/**
    \fn psHeader
    \brief constructor
*/

 psHeader::psHeader( void ) : vidHeader()
{ 
    interlaced=false;
    lastFrame=0xffffffff;
    
}
/**
    \fn psHeader
    \brief destructor
*/

 psHeader::~psHeader(  )
{
  close();
}

/**
        \fn getFrame
*/

uint8_t  psHeader::getFrame(uint32_t frame,ADMCompressedImage *img)
{
    if(frame>=ListOfFrames.size()) return 0;
    dmxFrame *pk=ListOfFrames[frame];
    if(frame==(lastFrame+1) && pk->type!=1) // the next frame, not an intra
    {
        lastFrame++;
        bool r=psPacket->read(pk->len,img->data);
             img->dataLength=pk->len;
             img->demuxerFrameNo=frame;
             img->demuxerDts=pk->dts;
             img->demuxerPts=pk->pts;
             //printf("[>>>] %d:%02x %02x %02x %02x\n",frame,img->data[0],img->data[1],img->data[2],img->data[3]);
             getFlags(frame,&(img->flags));
             return r;
    }
    if(pk->type==1) // an intra
    {
        if(!psPacket->seek(pk->startAt,pk->index)) return false;
         bool r=psPacket->read(pk->len,img->data);
             img->dataLength=pk->len;
             img->demuxerFrameNo=frame;
             img->demuxerDts=pk->dts;
             img->demuxerPts=pk->pts;
             getFlags(frame,&(img->flags));
             //printf("[>>>] %d:%02x %02x %02x %02x\n",frame,img->data[0],img->data[1],img->data[2],img->data[3]);
             lastFrame=frame;
             return r;
    }

    // a random frame: need to rewind first, then seek forward
    uint32_t startPoint=frame;
    while(startPoint && !ListOfFrames[startPoint]->startAt)
        startPoint--;
    printf("[psDemux] Wanted frame %" PRIu32", going back to frame %" PRIu32", last frame was %" PRIu32",\n",frame,startPoint,lastFrame);
    pk=ListOfFrames[startPoint];
    if(!psPacket->seek(pk->startAt,pk->index))
    {
        printf("[psDemux] Failed to rewind to frame %" PRIu32"\n",startPoint);
        return false;
    }

    // now seek forward
    while(startPoint<frame)
    {
        pk=ListOfFrames[startPoint];
        if(!psPacket->read(pk->len,img->data))
        {
            printf("[psDemux] Read failed for frame %" PRIu32"\n",startPoint);
            lastFrame=0xffffffff;
            return false;
        }
        startPoint++;
        lastFrame=startPoint;
    }
    pk=ListOfFrames[frame];
    lastFrame++;

    bool r=psPacket->read(pk->len,img->data);

    img->dataLength=pk->len;
    img->demuxerFrameNo=frame;
    img->demuxerDts=pk->dts;
    img->demuxerPts=pk->pts;
    //printf("[>>>] %d:%02x %02x %02x %02x\n",frame,img->data[0],img->data[1],img->data[2],img->data[3]);
    getFlags(frame,&(img->flags));
    return r;
}

/**
        \fn getExtraHeaderData
*/
uint8_t  psHeader::getExtraHeaderData(uint32_t *len, uint8_t **data)
{
                *len=0; //_tracks[0].extraDataLen;
                *data=NULL; //_tracks[0].extraData;
                return true;            
}
//EOF
