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
 * Module: EduOM_PrevObject.c
 *
 * Description: 
 *  Return the previous object of the given current object.
 *
 * Exports:
 *  Four EduOM_PrevObject(ObjectID*, ObjectID*, ObjectID*, ObjectHdr*)
 */


#include "EduOM_common.h"
#include "BfM.h"
#include "EduOM_Internal.h"

/*@================================
 * EduOM_PrevObject()
 *================================*/
/*
 * Function: Four EduOM_PrevObject(ObjectID*, ObjectID*, ObjectID*, ObjectHdr*)
 *
 * Description: 
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  Return the previous object of the given current object. Find the object in
 *  the same page which has the current object and  if there  is no previous
 *  object in the same page, find it from the previous page.
 *  If the current object is NULL, return the last object of the file.
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    some errors caused by function calls
 *
 * Side effect:
 *  1) parameter prevOID
 *     prevOID is filled with the previous object's identifier
 *  2) parameter objHdr
 *     objHdr is filled with the previous object's header
 */
Four EduOM_PrevObject(
    ObjectID *catObjForFile,	/* IN informations about a data file */
    ObjectID *curOID,		/* IN a ObjectID of the current object */
    ObjectID *prevOID,		/* OUT the previous object of a current object */
    ObjectHdr*objHdr)		/* OUT the object header of previous object */
{
    Four e;			/* error */
    Two  i;			/* index */
    Four offset;		/* starting offset of object within a page */
    PageID pid;			/* a page identifier */
    PageNo pageNo;		/* a temporary var for previous page's PageNo */
    SlottedPage *apage;		/* a pointer to the data page */
    Object *obj;		/* a pointer to the Object */
    SlottedPage *catPage;	/* buffer page containing the catalog object */
    sm_CatOverlayForData *catEntry; /* overlay structure for catalog object access */



    /*@ parameter checking */
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (prevOID == NULL) ERR(eBADOBJECTID_OM);

    e = BfM_GetTrain(catObjForFile, &catPage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);

    // 1-1. If curOID given as a parameter is NULL
    if (curOID == NULL){
        // 1-1-2. If the file is not empty : Return ID of the last object int the slot array of the file's last page
        if (catEntry->lastPage != NULL) {
            MAKE_PAGEID(pid, catEntry->fid.volNo, catEntry->lastPage);
            e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
            if (e < eNOERROR) ERR(e);

            i = apage->header.nSlots - 1;
            offset = apage->slot[-i].offset;
            obj = &(apage->data[offset]);

            MAKE_OBJECTID(*prevOID, pid.volNo, pid.pageNo, i, apage->slot[-i].unique);
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
        // 1-2-2-1. If the object found is the page's first obejct
        if (curOID->slotNo == 0){
            // 1-2-2-1-1. If the page is the first page of the file : return EOS
            // 1-2-2-1-2. there is previous page
            if (apage->header.pid.pageNo != catEntry->firstPage) {
                e = BfM_FreeTrain(&pid, PAGE_BUF);
                if (e < eNOERROR) ERR(e);
                MAKE_PAGEID(pid, curOID->volNo, apage->header.prevPage);
                e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
                if (e < eNOERROR) ERR(e);

                i = apage->header.nSlots - 1;
                offset = apage->slot[-i].offset;
                obj = &(apage->data[offset]);
                MAKE_OBJECTID(*prevOID, pid.volNo, pid.pageNo, i, apage->slot[-i].unique);
                objHdr = &(obj->header);
            }
        }
        // 1-2-2-2. If the object found is not the page's first object : return the previous object's id
        else {
            i = curOID->slotNo - 1;
            offset = apage->slot[-i].offset;
            obj = &(apage->data[offset]);
            MAKE_OBJECTID(*prevOID, pid.volNo, pid.pageNo, i, apage->slot[-i].unique);
            objHdr = &(obj->header);
        }

        e = BfM_FreeTrain(&pid, PAGE_BUF);
        if (e < eNOERROR) ERR(e);
    }

    e = BfM_FreeTrain(catObjForFile, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    return(EOS);
    
} /* EduOM_PrevObject() */
