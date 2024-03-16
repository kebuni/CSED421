# EduBfM Report

Name: Gyeongbhin Lim

Student id: 20180916

# Problem Analysis

## About ODYSSEUS/EduCOSMOS

* ODYSSEUS는 1990년부터 한국과학기술원 데이터베이스 및 멀티미디어 연구실에서 개발한
객체 관계형 DBMS이다.
* COSMOS는 ODYSSEUS의 저장시스템으로서 각종 데이터베이스 응용 소프트웨어의 하부 구조로 사용되고 있다. 
* EduCOSMOS는 COSMOS의 일부분을 구현하는 교육 목적용 프로젝트이다. COSMOS의
일부분을 구현함으로써 DBMS의 각 모듈 별 기능을 학습하는 것을 목표로 하고 있다.

## EduBfM project

* EduBfM은 EduCOSMOS 중에서 buffer를 관리하는 buffer manager 모듈이다.
Disk 상의 page 또는 여러 page로 이루어진 train을 main memory에 유지하는
기능을 수행한다.
* 디스크로부터 page 또는 train을 읽어 온 후 buffer에 할당하는 기능, buffer에
할당된 page 또는 train을 디스크에 작성하는 기능, page에 dirty bit를 set하는 
기능, buffer에 할당된 page 또는 train을 모두 할당 해제 하는 기능 등을 수행하는
API를 구현해야 한다.

# Design For Problem Solving

## High Level

* EduBfM_GetTrain(): train을 bufferpool에 fix하고, train이 저장된 buffer 
element에 대한 포인터를 반환함
* EduBfM_FreeTrain(): train을 bufferpool에서 unfix함
* EduBfM_SetDirty(): bufferpool에 저장된 train이 수정되었음을 표시하기 위해
Dirty bit을 1로 set함
* EduBfM_FlushAll(): 각 bufferPool에 존재하는 train 중 수정된 train을 disk에 
기록함
* EduBfM_DiscardAll(): 각 bufferPool에 존재하는 train들을 disk에 기록하지
않고 bufferPool에서 삭제함

## Low Level

* edubfm_ReadTrain(): train을 disk로부터 읽어와서 buffer element에 저장하고,
해당 buffer element에 대한 포인터를 반환함
* edubfm_AllocTrain(): bufferPool에서 train을 저장하기 위한 buffer element를
한 개 할당 받고, 해당 buffer element의 array index를 반환함
* edubfm_Insert(): hashTable에 buffer element의 array index를 삽입함
* edubfm_Delete(): hashTable에서 buffer element의 array index를 삭제함
* edubfm_DeleteAll(): 각 hashTable에서 모든 entry들을 삭제함
* edubfm_LookUp(): hashTable에서 파라미터로 주어진 hash key에 대응하는 buffer
element의 array index를 검색하여 반환함
* edubfm_FlushTrain(): 수정된 train을 disk에 기록함

# Mapping Between Implementation And the Design

## High Level

* EduBfM_GetTrain(): trainId에 해당하는 train을 return해야 하는 API이다.
우선 bufferpool에 해당 train이 존재하는 지 찾아야 한다. trainId로부터 
hashvalue를 얻어 hashtable을 통해 bufferpool에 train이 존재하는지 찾는다.
만약 존재한다면 fixed counter를 1 증가시키고 REFER bit를 set한 뒤 
bufferpool의 entry를 리턴한다. 만약 bufferpool에 존재하지 않는다면 
edubfm_AllocTrain을 이용해 디스크로부터 해당 train을 읽어온다. bufferInfo에
읽어온 train 관련 정보를 저장한 후 bufferpool의 entry를 리턴한다.
* EduBfM_FreeTrain(): trainId에 해당하는 bufferpool의 index를 검색하여 해당
train의 fixed counter를 1 감소시킨다. 이때 fixed counter가 0이 되면 
trainId를 출력한다.
* EduBfM_SetDirty(): 파라미터로 전달받은 trainId에 대하여 bufferpool에서의 
인덱스를 검색한 뒤 BufferInfo에 접근하여 Dirty bit를 set한다.
* EduBfM_FlushAll(): bufferpool에 있는 모든 entry에 대하여 만약 dirty bit가 
set되어 있을 경우 edubfm_FlushTrain()을 이용하여 변경사항을 디스크에 
flush한다. 이후 Dirty bit를 다시 reset해야 한다.
* EduBfM_DiscardAll(): bufferpool에 있는 모든 entry에 대하여 bufferInfo를
초기화해야 한다. 이때 NextHashEntry는 초기화할 필요가 없다. 이후 
edubfm_DeleteAll()을 이용하여 hashtable도 초기화한다.

## Low Level

* edubfm_ReadTrain(): RDsM_ReadTrain()을 이용하여 parameter로 전달받은
trainId에 해당하는 train을 디스크로부터 읽고 buffer를 가리키는 pointer인
aTrain의 값을 변경한다.
* edubfm_AllocTrain(): evict할 buffer index를 선정하기 위해 BufInfo에 
저장된 NextVictim으로부터 인덱스를 가져와 조사한다. 인덱스를 1씩 더해가면서
fixed counter가 0일 경우, refer bits가 set되어 있는지 조사한다. 만약 refer
bit가 1이라면 reset하고 다음 인덱스를 조사한다. 만약 refer bit가 0이라면 
해당 인덱스를 victim으로 선정한다. 해당 buffer가 dirty bit가 1일 경우에는
edubfm_FlushTrain()을 이용하여 변경사항을 disk에 작성한다. edubfm_Delete()
를 이용하여 hashtable로부터 victim의 인덱스 정보를 삭제한다. NextVictim을
선정된 victim의 다음 인덱스로 설정하고 victim을 리턴한다.
* edubfm_Insert(): hash값이 동일한 entry들의 link가 유지될 수 있도록 새로
할당한 train에 대한 buffer에서의 index를 hashtable에 넣고, 만약 기존에 
hashtable entry가 있었다면 새로 할당한 train의 NextHashEntry로 저장한다.
* edubfm_Delete(): parameter로 전달받은 key로부터 hashValue를 이용해 
hashtable entry를 얻는다. 만약 hashtable의 entry가 NIL 값으로 아직 buffer
index가 저장되지 않을 상황일 경우 eNOTFOUND_BFM 에러코드를 리턴한다.
이후 key와 buffer의 index에 해당하는 key가 같아질 때까지 NextHashEntry를
조회한 후, hash값이 동일한 entry들의 link가 유지될 수 있도록 이전 index의
NextHashEntry로 삭제하고자 하는 buffer entry의 NextHashEntry를 대입한다.
* edubfm_DeleteAll(): 모든 type와 모든 hashtable의 entry를 NIL로 초기화한다.
* edubfm_LookUp(): parameter로 전달받은 key로부터 hashValue를 이용해 
hashtable entry를 얻는다. 이후 key와 buffer의 index에 해당하는 key가 같아질 
때까지 NextHashEntry를 조회한 후 찾고자 하는 key가 존재하는 buffer에서의 
index를 리턴한다.
* edubfm_FlushTrain(): edubfm_LookUp()을 이용해 buffer 상에서 찾고자 하는
train의 인덱스를 찾은 후 RDsM_WriteTrain()을 이용하여 disk에 buffer의 내용을
flush한다.