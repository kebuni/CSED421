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
 * Module : EduOM_DestroyObject.c
 * 
 * Description : 
 *  EduOM_DestroyObject() destroys the specified object.
 *
 * Exports:
 *  Four EduOM_DestroyObject(ObjectID*, ObjectID*, Pool*, DeallocListElem*)
 */

#include "EduOM_common.h"
#include "Util.h"		/* to get Pool */
#include "RDsM.h"
#include "BfM.h"		/* for the buffer manager call */
#include "LOT.h"		/* for the large object manager call */
#include "EduOM_Internal.h"

/*@================================
 * EduOM_DestroyObject()
 *================================*/
/*
 * Function: Four EduOM_DestroyObject(ObjectID*, ObjectID*, Pool*, DeallocListElem*)
 * 
 * Description : 
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  (1) What to do?
 *  EduOM_DestroyObject() destroys the specified object. The specified object
 *  will be removed from the slotted page. The freed space is not merged
 *  to make the contiguous space; it is done when it is needed.
 *  The page's membership to 'availSpaceList' may be changed.
 *  If the destroyed object is the only object in the page, then deallocate
 *  the page.
 *
 *  (2) How to do?
 *  a. Read in the slotted page
 *  b. Remove this page from the 'availSpaceList'
 *  c. Delete the object from the page
 *  d. Update the control information: 'unused', 'freeStart', 'slot offset'
 *  e. IF no more object in this page THEN
 *	   Remove this page from the filemap List
 *	   Dealloate this page
 *    ELSE
 *	   Put this page into the proper 'availSpaceList'
 *    ENDIF
 * f. Return
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    eBADFILEID_OM
 *    some errors caused by function calls
 */
Four EduOM_DestroyObject(
    ObjectID *catObjForFile,	/* IN file containing the object */
    ObjectID *oid,		/* IN object to destroy */
    Pool     *dlPool,		/* INOUT pool of dealloc list elements */
    DeallocListElem *dlHead)	/* INOUT head of dealloc list */
{
    Four        e;		/* error number */
    Two         i;		/* temporary variable */
    FileID      fid;		/* ID of file where the object was placed */
    PageID	pid;		/* page on which the object resides */
    SlottedPage *apage;		/* pointer to the buffer holding the page */
    Four        offset;		/* start offset of object in data area */
    Object      *obj;		/* points to the object in data area */
    Four        alignedLen;	/* aligned length of object */
    Boolean     last;		/* indicates the object is the last one */
    SlottedPage *catPage;	/* buffer page containing the catalog object */
    sm_CatOverlayForData *catEntry; /* overlay structure for catalog object access */
    DeallocListElem *dlElem;	/* pointer to element of dealloc list */
    PhysicalFileID pFid;	/* physical ID of file */
    
    

    /*@ Check parameters. */
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);

    if (oid == NULL) ERR(eBADOBJECTID_OM);

    // 1. Delete the page storing the object to be deleted from the currently available space list
    e = BfM_GetTrain(catObjForFile, (char**)&catPage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);
    MAKE_PHYSICALFILEID(pFid, catEntry->fid.volNo, catEntry->firstPage);
    
    MAKE_PAGEID(pid, oid->volNo, oid->pageNo);
    e = BfM_GetTrain(&pid, (char**)&apage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    e = om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
    if (e < eNOERROR) ERR(e);
    obj = &(apage->data[apage->slot[-(oid->slotNo)].offset]);

    // 2. Set the slot corresponding to the object deleted as an empty unused slot
    apage->slot[-(oid->slotNo)].offset = EMPTYSLOT;
    alignedLen = ALIGNED_LENGTH(obj->header.length);

    // 3. Update the page header
    last = FALSE;
    if (oid->slotNo + 1 == apage->header.nSlots)
        last = TRUE;
    // 3-1. IF the slot corresponding to the object deleted is the last slot of the slot array, update ths size of slot array
    // 3-2. Update the variables free or unused depending on the offset in the data area of the object deleted.
    if (last == TRUE){
        // The size of the free space obtained by deleting the object : 
        // sizeof(ObjectHdr) + size of the aligned data area + size of the free space by updating slot array size
        apage->header.nSlots -= 1;
        apage->header.free -= sizeof(ObjectHdr) + alignedLen;
    }
    else{
        apage->header.unused += sizeof(ObjectHdr) + alignedLen;
    }

    // 4-1. If the deleted object is the only object in the page, and the page is not the first page of the file
    if (apage->header.nSlots == 0 && apage->header.pid.pageNo != catEntry->firstPage){
        
        // 4-1-1. Delete the page from the list of pages of the file
        e = om_FileMapDeletePage(catObjForFile, &pid);
        if (e < eNOERROR) ERR(e);
        // 4-1-2. Deallocate the page
        // 4-1-2-1. Allocate a new dealloc list element from the dlPool given as a parameter
        e = Util_getElementFromPool(dlPool, &dlElem);
        if (e < eNOERROR) ERR(e);
        // 4-1-2-2. Store the information about the pages to be deallocated into the element allocated.
        dlElem->type = DL_PAGE;
        //dlElem->elem.pFid = pFid;
        dlElem->elem.pid = pid;
        // 4-1-2-3. Insert the element into the dealloc list as the first element.
        dlElem->next = dlHead->next;
        dlHead->next = dlElem;        
    }
    // 4-2. If the deledted object is not the only object of a page or if the page is the first page of the file
    else {
        // 4-2-1. Insert the page into the appropriate available space list
        om_PutInAvailSpaceList(catObjForFile, &pid, apage);
        if (e < eNOERROR) ERR(e);
    }
    
    e = BfM_SetDirty(&pid, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    e = BfM_FreeTrain(&pid, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    e = BfM_FreeTrain(catObjForFile, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    return(eNOERROR);
    
} /* EduOM_DestroyObject() */
