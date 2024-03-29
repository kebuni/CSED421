/******************************************************************************/
/*                                                                            */
/*    ODYSSEUS/EduCOSMOS Educational-Purpose Object Storage System            */
/*                                                                            */
/*    Developed by Professor Kyu-Young Whang et al.                           */
/*                                                                            */
/*    Database and Multimedia Laboratory                                      */
/*                                                                            */
/*    Computer Science Department and                                         */
/*    Advanced Information Technology Research Center (AITrc)                 */
/*    Korea Advanced Institute of Science and Technology (KAIST)              */
/*                                                                            */
/*    e-mail: kywhang@cs.kaist.ac.kr                                          */
/*    phone: +82-42-350-7722                                                  */
/*    fax: +82-42-350-8380                                                    */
/*                                                                            */
/*    Copyright (c) 1995-2013 by Kyu-Young Whang                              */
/*                                                                            */
/*    All rights reserved. No part of this software may be reproduced,        */
/*    stored in a retrieval system, or transmitted, in any form or by any     */
/*    means, electronic, mechanical, photocopying, recording, or otherwise,   */
/*    without prior written permission of the copyright owner.                */
/*                                                                            */
/******************************************************************************/
/*
 * Module : EduOM_CreateObject.c
 * 
 * Description :
 *  EduOM_CreateObject() creates a new object near the specified object.
 *
 * Exports:
 *  Four EduOM_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four, char*, ObjectID*)
 */

#include <string.h>
#include "EduOM_common.h"
#include "RDsM.h"		/* for the raw disk manager call */
#include "BfM.h"		/* for the buffer manager call */
#include "EduOM_Internal.h"

/*@================================
 * EduOM_CreateObject()
 *================================*/
/*
 * Function: Four EduOM_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four, char*, ObjectID*)
 * 
 * Description :
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 * (1) What to do?
 * EduOM_CreateObject() creates a new object near the specified object.
 * If there is no room in the page holding the specified object,
 * it trys to insert into the page in the available space list. If fail, then
 * the new object will be put into the newly allocated page.
 *
 * (2) How to do?
 *	a. Read in the near slotted page
 *	b. See the object header
 *	c. IF large object THEN
 *	       call the large object manager's lom_ReadObject()
 *	   ELSE 
 *		   IF moved object THEN 
 *				call this function recursively
 *		   ELSE 
 *				copy the data into the buffer
 *		   ENDIF
 *	   ENDIF
 *	d. Free the buffer page
 *	e. Return
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADLENGTH_OM
 *    eBADUSERBUF_OM
 *    some error codes from the lower level
 *
 * Side Effects :
 *  0) A new object is created.
 *  1) parameter oid
 *     'oid' is set to the ObjectID of the newly created object.
 */
Four EduOM_CreateObject(
    ObjectID  *catObjForFile,	/* IN file in which object is to be placed */
    ObjectID  *nearObj,		/* IN create the new object near this object */
    ObjectHdr *objHdr,		/* IN from which tag is to be set */
    Four      length,		/* IN amount of data */
    char      *data,		/* IN the initial data for the object */
    ObjectID  *oid)		/* OUT the object's ObjectID */
{
    Four        e;		/* error number */
    ObjectHdr   objectHdr;	/* ObjectHdr with tag set from parameter */


    /*@ parameter checking */
    
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (length < 0) ERR(eBADLENGTH_OM);

    if (length > 0 && data == NULL) return(eBADUSERBUF_OM);

	/* Error check whether using not supported functionality by EduOM */
	if(ALIGNED_LENGTH(length) > LRGOBJ_THRESHOLD) ERR(eNOTSUPPORTED_EDUOM);
    
    objectHdr.properties = 0x0;
    objectHdr.length = 0;
    if (objHdr == NULL)
        objectHdr.tag = 0;
    else
        objectHdr.tag = objHdr->tag;

    e = eduom_CreateObject(catObjForFile, nearObj, &objectHdr, length, data, oid);
    if (e != eNOERROR)
        ERR(e);
    
    return(eNOERROR);
}

/*@================================
 * eduom_CreateObject()
 *================================*/
/*
 * Function: Four eduom_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four, char*, ObjectID*)
 *
 * Description :
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  eduom_CreateObject() creates a new object near the specified object; the near
 *  page is the page holding the near object.
 *  If there is no room in the near page and the near object 'nearObj' is not
 *  NULL, a new page is allocated for object creation (In this case, the newly
 *  allocated page is inserted after the near page in the list of pages
 *  consiting in the file).
 *  If there is no room in the near page and the near object 'nearObj' is NULL,
 *  it trys to create a new object in the page in the available space list. If
 *  fail, then the new object will be put into the newly allocated page(In this
 *  case, the newly allocated page is appended at the tail of the list of pages
 *  cosisting in the file).
 *
 * Returns:
 *  error Code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    some errors caused by fuction calls
 */
Four eduom_CreateObject(
                        ObjectID	*catObjForFile,	/* IN file in which object is to be placed */
                        ObjectID 	*nearObj,	/* IN create the new object near this object */
                        ObjectHdr	*objHdr,	/* IN from which tag & properties are set */
                        Four	length,		/* IN amount of data */
                        char	*data,		/* IN the initial data for the object */
                        ObjectID	*oid)		/* OUT the object's ObjectID */
{
    Four        e;		/* error number */
    Four	neededSpace;	/* space needed to put new object [+ header] */
    SlottedPage *apage;		/* pointer to the slotted page buffer */
    Four        alignedLen;	/* aligned length of initial data */
    Boolean     needToAllocPage;/* Is there a need to alloc a new page? */
    PageID      pid;            /* PageID in which new object to be inserted */
    PageID      nearPid;
    Four        firstExt;	/* first Extent No of the file */
    Object      *obj;		/* point to the newly created object */
    Two         i;		/* index variable */
    sm_CatOverlayForData *catEntry; /* pointer to data file catalog information */
    SlottedPage *catPage;	/* pointer to buffer containing the catalog */
    FileID      fid;		/* ID of file where the new object is placed */
    Two         eff;		/* extent fill factor of file */
    Boolean     AvailSpaceListExist;
    PhysicalFileID pFid;

    

    /*@ parameter checking */
    
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (objHdr == NULL) ERR(eBADOBJECTID_OM);
    
    /* Error check whether using not supported functionality by EduOM */
    if(ALIGNED_LENGTH(length) > LRGOBJ_THRESHOLD) ERR(eNOTSUPPORTED_EDUOM);
    
    // 1. Calculate the size of the free space required for inserting the object
    alignedLen = ALIGNED_LENGTH(length);
    neededSpace = sizeof(ObjectHdr) + sizeof(SlottedPageSlot) + alignedLen;

    // 2. Select a page to insert the object
    e = BfM_GetTrain(catObjForFile, (char**)&catPage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);
    MAKE_PHYSICALFILEID(pFid, catEntry->fid.volNo, catEntry->firstPage);
    e = RDsM_PageIdToExtNo(&pFid, &firstExt);
    if (e < eNOERROR) ERR(e);

    // 2-1. nearObj given as a parameter is not NULL
    if (nearObj != NULL){
        MAKE_PAGEID(nearPid, nearObj->volNo, nearObj->pageNo);
        e = BfM_GetTrain(&nearPid, (char**)&apage, PAGE_BUF);
        if (e < eNOERROR) ERR(e);
        // 2-1-1. there is available space in the nearPage
        if (SP_FREE(apage) >= neededSpace){
            // Select the page as the page to insert the object.
            MAKE_PAGEID(pid, nearPid.volNo, nearPid.pageNo);
            // Delete the selected page from the current available space list
            e = om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
            if (e < eNOERROR) ERR(e);
            // Compact the selected page if necessary
            if (SP_CFREE(apage) < neededSpace) {
                e = OM_CompactPage(apage, nearObj->slotNo);
                if(e < eNOERROR) ERR(e);
            }
        }
        // 2-1-2. there is no avilable space in the nearPage
        else {
            e = BfM_FreeTrain(&nearPid, PAGE_BUF);
            if (e < eNOERROR) ERR(e);
            // Allocate a new page to insert the object into.
            e = RDsM_AllocTrains(catEntry->fid.volNo, firstExt, &nearPid, catEntry->eff, 1, PAGESIZE2, &pid);
            if (e < eNOERROR) ERR(e);
            // Initialize the selected page's header
            e = BfM_GetNewTrain(&pid, (char**)&apage, PAGE_BUF);
            if (e < eNOERROR) ERR(e);
            apage->header.flags = 0x2;
            apage->header.free = 0;
            apage->header.unused = 0;
            apage->header.fid = catEntry->fid;
            // Insert the page as the next page of the page stroing nearObj into the list of pages of the file.
            e = om_FileMapAddPage(catObjForFile, &nearPid, &pid);
            if (e < eNOERROR) ERR(e);
        }
    }
    // 2-2. nearObj given as a parameter is NULL
    else {
        PageNo *availListPageNo = -1;
        AvailSpaceListExist = FALSE;
        if (neededSpace <= SP_10SIZE){
            availListPageNo = catEntry->availSpaceList10;
            MAKE_PAGEID(pid, pFid.volNo, catEntry->availSpaceList10);
            AvailSpaceListExist = TRUE;
        }  
        else if (neededSpace <= SP_20SIZE){
            availListPageNo = catEntry->availSpaceList10;
            MAKE_PAGEID(pid, pFid.volNo, catEntry->availSpaceList10);
            AvailSpaceListExist = TRUE;
        }       
        else if (neededSpace <= SP_30SIZE){
            availListPageNo = catEntry->availSpaceList20;
            MAKE_PAGEID(pid, pFid.volNo, catEntry->availSpaceList20);
            AvailSpaceListExist = TRUE;
        }    
        else if (neededSpace <= SP_40SIZE){
            availListPageNo = catEntry->availSpaceList30;
            MAKE_PAGEID(pid, pFid.volNo, catEntry->availSpaceList30);
            AvailSpaceListExist = TRUE;
        }  
        else if (neededSpace <= SP_50SIZE){
            availListPageNo = catEntry->availSpaceList40;
            MAKE_PAGEID(pid, pFid.volNo, catEntry->availSpaceList40);
            AvailSpaceListExist = TRUE;
        }
        else {
            availListPageNo = catEntry->availSpaceList50;
            MAKE_PAGEID(pid, pFid.volNo, catEntry->availSpaceList50);
            AvailSpaceListExist = TRUE;
        }

        // 2-2-1. there is an available space list
        if (availListPageNo != -1){
        //if (AvailSpaceListExist == TRUE){
            // select the first page of the corresponding available space list
            //pid.volNo = pFid.volNo;
            MAKE_PAGEID(pid, pFid.volNo, availListPageNo);
            e = BfM_GetTrain(&pid, (char**)&apage, PAGE_BUF);
            if (e < eNOERROR) ERR(e);
            // delete the page from current available space list
            e = om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
            if (e < eNOERROR) ERR(e);
            // compact the selected page if necessary
            if (SP_CFREE(apage) < neededSpace) {
                e = OM_CompactPage(apage, nearObj->slotNo);
                if(e < eNOERROR) ERR(e);
            }
        }
        // 2-2-2. there is no available space list
        else {
            MAKE_PAGEID(pid, pFid.volNo, catEntry->lastPage);
            e = BfM_GetTrain(&pid, (char**)&apage, PAGE_BUF);
            if (e < eNOERROR) ERR(e);
            // 2-2-2-1. there is available space in the file's last page
            if (SP_FREE(apage) >= neededSpace){        
                e = om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
                if(e < 0) ERR(e);        
                // compact the selected page if necessary
                if (SP_CFREE(apage) < neededSpace) {
                    e = OM_CompactPage(apage, nearObj->slotNo);
                    if(e < eNOERROR) ERR(e);
                }   
            }
            // 2-2-2-2. there is no available space in the file's last page
            else {
                e = BfM_FreeTrain(&pid, PAGE_BUF);
                if (e < eNOERROR) ERR(e);
                MAKE_PAGEID(nearPid, pFid.volNo, catEntry->lastPage);
                // Allocate a new page to insert the object to
                e = RDsM_AllocTrains(catEntry->fid.volNo, firstExt, &nearPid, catEntry->eff, 1, PAGESIZE2, &pid);
                if (e < eNOERROR) ERR(e);
                // Initialize the page's header
                e = BfM_GetNewTrain(&pid, (char**)&apage, PAGE_BUF);
                if (e < eNOERROR) ERR(e);
                apage->header.flags = 0x2;
                apage->header.free = 0;
                apage->header.unused = 0;
                apage->header.fid = catEntry->fid;
                // Insert the page as the last page into the list of pages of the file.
                e = om_FileMapAddPage(catObjForFile, &(catEntry->lastPage), &pid);
                if (e < eNOERROR) ERR(e);
            }
        }
        
    }
    
    // 3. Insert an object into the selected page
    i = 0;
    while (apage->slot[-i].offset != EMPTYSLOT && i < apage->header.nSlots){
        i++;
    }

    // 3-0. Allocate an empty slot of the slot array
    SlottedPageSlot* newSlot = &(apage->slot[-i]);
    newSlot->offset = apage->header.free;

    // 3-1. Update the object's header
    obj = &(apage->data[newSlot->offset]);
    obj->header.properties = objHdr->properties;
    obj->header.tag = objHdr->tag;
    obj->header.length = length;
    e = om_GetUnique(&pid, &(newSlot->unique));
    if (e < eNOERROR) ERR(e);
    MAKE_OBJECTID(*oid, pid.volNo, pid.pageNo, i, newSlot->unique);
    
    // 3-2. Copy the object into the selected page's contiguous free area
    memcpy(obj->data, data, length);

    // 3-3. Update the page's header 
    apage->header.free += sizeof(ObjectHdr) + alignedLen;
    apage->header.nSlots = MAX(apage->header.nSlots, i+1);
    //apage->header.unique = newSlot->unique;

    e = BfM_SetDirty((TrainID*)&pid, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    e = om_PutInAvailSpaceList(catObjForFile, &pid, apage);
    if (e < eNOERROR) ERR(e);
    e = BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    e = BfM_FreeTrain((TrainID*)catObjForFile, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    // 4. Return the ID of the object inserted
    return(eNOERROR);

} /* eduom_CreateObject() */
