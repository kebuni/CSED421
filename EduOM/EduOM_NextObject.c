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
 * Module: EduOM_NextObject.c
 *
 * Description:
 *  Return the next Object of the given Current Object. 
 *
 * Export:
 *  Four EduOM_NextObject(ObjectID*, ObjectID*, ObjectID*, ObjectHdr*)
 */


#include "EduOM_common.h"
#include "BfM.h"
#include "EduOM_Internal.h"

/*@================================
 * EduOM_NextObject()
 *================================*/
/*
 * Function: Four EduOM_NextObject(ObjectID*, ObjectID*, ObjectID*, ObjectHdr*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  Return the next Object of the given Current Object.  Find the Object in the
 *  same page which has the current Object and  if there  is no next Object in
 *  the same page, find it from the next page. If the Current Object is NULL,
 *  return the first Object of the file.
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    some errors caused by function calls
 *
 * Side effect:
 *  1) parameter nextOID
 *     nextOID is filled with the next object's identifier
 *  2) parameter objHdr
 *     objHdr is filled with the next object's header
 */
Four EduOM_NextObject(
    ObjectID  *catObjForFile,	/* IN informations about a data file */
    ObjectID  *curOID,		/* IN a ObjectID of the current Object */
    ObjectID  *nextOID,		/* OUT the next Object of a current Object */
    ObjectHdr *objHdr)		/* OUT the object header of next object */
{
    Four e;			/* error */
    Two  i;			/* index */
    Four offset;		/* starting offset of object within a page */
    PageID pid;			/* a page identifier */
    PageNo pageNo;		/* a temporary var for next page's PageNo */
    SlottedPage *apage;		/* a pointer to the data page */
    Object *obj;		/* a pointer to the Object */
    PhysicalFileID pFid;	/* file in which the objects are located */
    SlottedPage *catPage;	/* buffer page containing the catalog object */
    sm_CatOverlayForData *catEntry; /* data structure for catalog object access */



    /*@
     * parameter checking
     */
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (nextOID == NULL) ERR(eBADOBJECTID_OM);

    e = BfM_GetTrain(catObjForFile, &catPage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);

    // 1-1. If curOID given as a parameter is NULL
    if (curOID == NULL){
        // 1-1-2. If the file is not empty : Return ID of the first object int the slot array of the file's first page
        if (catEntry->firstPage != NULL) {
            MAKE_PAGEID(pid, catEntry->fid.volNo, catEntry->firstPage);
            e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
            if (e < eNOERROR) ERR(e);

            i = 0;
            offset = apage->slot[-i].offset;
            obj = &(apage->data[offset]);

            MAKE_OBJECTID(*nextOID, pid.volNo, pid.pageNo, i, apage->slot[-i].unique);
            objHdr = &(obj->header);

            e = BfM_FreeTrain(&pid, PAGE_BUF);
            if (e < eNOERROR) ERR(e);
        }
    }
    // 1-2. If curOID given as the parameter is not NULL
    else {
        // 1-2-1. search for an object corresponding to curOID
        MAKE_PAGEID(pid, curOID->volNo, curOID->pageNo);
        e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
        if (e < eNOERROR) ERR(e);
        // 1-2-2-1. If the object found is the page's last obejct
        if (curOID->slotNo == apage->header.nSlots - 1){
            // 1-2-2-1-1. If the page is the last page of the file : return EOS
            // 1-2-2-1-2. there is next page
            if (apage->header.pid.pageNo != catEntry->lastPage) {
                e = BfM_FreeTrain(&pid, PAGE_BUF);
                if (e < eNOERROR) ERR(e);
                MAKE_PAGEID(pid, curOID->volNo, apage->header.nextPage);
                e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
                if (e < eNOERROR) ERR(e);

                i = 0;
                offset = apage->slot[-i].offset;
                obj = &(apage->data[offset]);
                MAKE_OBJECTID(*nextOID, pid.volNo, pid.pageNo, i, apage->slot[-i].unique);
                objHdr = &(obj->header);
            }
        }
        // 1-2-2-2. If the object found is not the page's last object : return the next object's id
        else {
            i = curOID->slotNo + 1;
            offset = apage->slot[-i].offset;
            obj = &(apage->data[offset]);
            MAKE_OBJECTID(*nextOID, pid.volNo, pid.pageNo, i, apage->slot[-i].unique);
            objHdr = &(obj->header);
        }

        e = BfM_FreeTrain(&pid, PAGE_BUF);
        if (e < eNOERROR) ERR(e);
    }

    e = BfM_FreeTrain(catObjForFile, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    return(EOS);		/* end of scan */
    
} /* EduOM_NextObject() */
