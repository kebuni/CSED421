# EduOM Report

Name: Lim GyeongBhin

Student id: 20180916

# Problem Analysis

* EduOM Project는 EduCOSMOS에서 object를 저장하기 위한 slotted page 관련 구조에 대한 operation들을 구현하는 프로젝트이다.
* sm_CatOverlayForData는 데이터 file에 대한 정보를 저장하기 위한 데이터 구조이며,
file의 첫페이지, 마지막 페이지 그리고 페이지 내 available space의 크기 정도에 따라 나눈
availSpaceList를 관리하고 있다. 때문에 오브젝트를 새롭게 만들거나 삭제할 때 이러한 file
구조가 일관성을 가지도록 업데이트하는 것이 필요하다.
* file 내 각 page는 slotted page로 구성되어 있어 페이지 헤더, 실제 데이터가 들어있는
데이터, 슬롯으로 구성되어 있다. 역시 오브젝트를 새롭게 만들거나 삭제할 때 이런 헤더, 
슬롯, 그리고 데이터 저장 위치가 잘 관리되도록 API를 구성해야 한다.
* 만약 오브젝트 사이 쓰지 않는 공간이 크다면 오브젝트의 위치를 조정하여 연속된 free 
space 공간을 최대한 확보할 수 있도록 operation을 구현해야 한다.
* 또한 어떠한 오브젝트가 주어졌을 때 그 오브젝트의 이전 오브젝트, 다음 오브젝트에 접근
할 수 있는데, 이때 페이지의 끝 오브젝트이거나 파일의 끝 오브젝트일 경우 처리가 필요하다.     

# Design For Problem Solving

## High Level

구현해야 할 각 operation의 high level design은 다음과 같다.
* Create an object
    * File을 구성하는 page들 중 파라미터로 지정한 object와 같은
    또는 인접한 페이지에 새로운 오브젝트를 생성하고 생성된 object의 ID를 반환한다.
    * 삽입할 오브젝트의 헤더를 만들어 초기화한다.
    * 페이지에 생성한 오브젝트를 삽입하고 삽입된 오브젝트의 ID를 반환한다.
* Remove an object
    * 파일을 구성하는 페이지에서 오브젝트를 삭제하고 페이지 헤더를 업데이트한다.
    * 사용하지 않는 슬롯을 빈 슬롯으로 설정하여 다음에 다른 오브젝트가 생성될
    경우 쓸 수 있게 한다.
* Compact a slotted page
    * 페이지의 데이터 영역의 모든 free space가 연속된 하나의 공간을 이루도록
    오브젝트들의 오프셋을 조정하여 페이지의 공간 효율을 높인다.
    * 페이지의 모든 슬롯을 조회한 후, 사용하는 슬롯일 경우 해당되는 오브젝트의
    오프셋을 최대한 앞으로 끌어온다.
* Read data of an object
    * 오브젝트의 데이터 전체 또는 일부를 읽고, 읽은 데이터에 대한 포인터를 반환한다.
    * 파라미터로 주어진 oid를 이용하여 오브젝트에 접근한다.
    * 파라미터로 주어진 start와 length를 잘 고려하여 object의 데이터를 카피한다.
* Access to previous object of given object
    * 파라미터로 주어진 오브젝트의 이전 오브젝트 ID를 반환한다.
    * 오브젝트가 페이지 또는 파일 가장 앞 오브젝트일 경우 예외처리를 해준다.
* Access to next object of given object
    * 파라미터로 주어진 오브젝트의 다음 오브젝트 ID를 반환한다.
    * 오브젝트가 페이지 또는 파일 가장 뒤 오브젝트일 경우 예외처리를 해준다.

## Low Level

위 operation은 구체적으로 다음과 같이 design하였다.
* Create an object
    * 파일 정보를 담고 있는 페이지를 메모리에 할당하여 불러온다.
    * 파일에 대한 정보를 담은 catEntry의 포인터를 가져온다.
    * nearObj 존재여부와, nearObj가 존재하는 페이지의 정보에 따라서 새로운 오브젝트가 생성될 페이지를 선정한다.
        * 만약 nearObj가 존재하는 경우, 해당 페이지가 새로운 오브젝트를 수용할 만한 크기가 되는지 판단한 후,
        그렇지 않다면 새로운 페이지를 할당한다.
        * 만약 nearObj가 존재하지 않는 경우, 생성하고자 하는 오브젝트의 크기를 수용할만한 가장 작은 크기의 available
        space list의 페이지를 가져온다. 없다면 파일 마지막 페이지에 생성하거나 새로운 페이지를 생성한다.
    * 페이지 선정이 끝났다면 새로운 오브젝트를 생성하고 헤더를 초기화한 후 해당 페이지에 오브젝트를 삽입한다.
    * 페이지를 다시 적당한 available space list에 넣고, 메모리에 올린 페이지들을 다시 해제한다.
* Remove an object
    * 파일을 구성하는 페이지에서 오브젝트를 삭제하고 페이지 헤더를 업데이트한다.
    * 삭제할 오브젝트가 저장된 페이지를 available space list에서 삭제한다.
    * 페이지 헤더를 업데이트한다. 
        * 만약 삭제할 오브젝트에 대응하는 슬롯이 마지막 슬롯일 경우 nSlots를 줄여야 한다.
        * 삭제한 오브젝트로 인해 늘어난 free space를 업데이트한다.
    * 오브젝트가 페이지의 유일한 오브젝트였을 경우 해당 페이지를 파일에서 삭제해야 한다.
        * 이때 dealloc list에 page 정보를 추가해야 한다.
    * 이후 새롭게 available space list에 추가한다.
* Compact a slotted page
    * 페이지의 데이터 영역의 모든 free space가 연속된 하나의 공간을 이루도록
    오브젝트들의 오프셋을 조정하여 페이지의 공간 효율을 높인다.
    * 페이지의 모든 슬롯을 차례대로 살펴 보면서
        * 만약 슬롯의 offset이 -1, 즉 비어있는 슬롯일 경우 다음으로 넘어간다.
        * 만약 슬롯의 offset이 -1, 즉 비어있는 슬롯이 아닐 경우 마지막으로 정렬한 오브젝트의 이후 위치로 오프셋을 
        옮기고 데이터를 다시 저장한다.
    * 연속된 free space 크기를 반영하여 페이지 헤더를 업데이트한다.
* Read data of an object
    * 오브젝트의 데이터 전체 또는 일부를 읽고, 읽은 데이터에 대한 포인터를 반환한다.
    * 파라미터로 주어진 oid를 이용하여 object에 접근한다.
    * 파라미터로 주어진 start 및 length를 고려하여 접근한 오브젝트의 데이터를 읽는다.
        * 오브젝트의 데이터 영역 상에서 start와 length의 길이의 합이 오브젝트의 데이터 크기보다 더 클 경우,
        error 처리를 한다.
        * length가 REMAINDER로 설정되어 있는 경우, 마지막 
        데이터까지 읽도록 처리한다.
    * 결정된 length만큼 오브젝트로부터 데이터를 카피하고,
    length를 리턴한다.
* Access to previous object of given object
    * 파라미터로 주어진 오브젝트의 이전 오브젝트 ID를 반환한다.
    * 파라미터로 주어진 curOID가 NULL인 경우, 파일의 마지막 페이지의 마지막 오브젝트의 ID를 반환한다.
    * 파라미터로 주어진 curOID가 NULL이 아닌 경우,
        * 현재 오브젝트가 페이지의 첫 오브젝트인 경우, 이전 페이지의 마지막 오브젝트를 반환한다.
        * 현재 오브젝트가 페이지의 첫 오브젝트가 아닌 경우,
        페이지 내 이전 오브젝트를 반환한다.
* Access to next object of given object
    * 파라미터로 주어진 오브젝트의 다음 오브젝트 ID를 반환한다.
    * 파라미터로 주어진 curOID가 NULL인 경우, 파일의 첫번째 페이지의 첫번째 오브젝트의 ID를 반환한다.
    * 파라미터로 주어진 curOID가 NULL이 아닌 경우,
        * 현재 오브젝트가 페이지의 마지막 오브젝트인 경우, 다음 페이지의 첫 오브젝트를 반환한다.
        * 현재 오브젝트가 페이지의 마지막 오브젝트가 아닌 경우,
        페이지 내 다음 오브젝트를 반환한다.

# Mapping Between Implementation And the Design

위 operation들은 각각 다음 함수들과 대응된다.

* Create an object : EduOM_CreateObject()
* Remove an object : EduOM_DestroyObject()
* Compact a slotted page : EduOM_CompactPage()
* Read data of an object : EduOM_ReadObject()
* Access to previous object of given object : EduOM_NextObject()
* Access to next object of given object : EduOM_PrevObject()