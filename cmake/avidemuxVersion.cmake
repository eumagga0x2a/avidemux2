SET(RELEASE 1)
include(admTimeStamp)
SET(CPACK_PACKAGE_VERSION_MAJOR "2")
SET(CPACK_PACKAGE_VERSION_MINOR "6")
SET(CPACK_PACKAGE_VERSION_PATCH "9")
SET(AVIDEMUX_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}") 
IF(NOT RELEASE)
        # use date instead of subversion, we use git now
        ADM_TIMESTAMP(date)
        SET(CPACK_PACKAGE_VERSION_PATCH "${CPACK_PACKAGE_VERSION_PATCH}-${date}")
ENDIF(NOT RELEASE)
