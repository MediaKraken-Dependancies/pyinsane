#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <windows.h>
#include <atlbase.h>
#include <wia.h>
#include <Sti.h>

#include <Python.h>

#define WIA_WARNING(msg) do { \
        PyErr_WarnEx(NULL, (msg), 1); \
        fprintf(stderr, (msg)); \
        fprintf(stderr, "\n"); \
    } while(0)

#define WIA_PYCAPSULE_DEV_NAME "WIA device"
#define WIA_PYCAPSULE_SRC_NAME "WIA source"

struct wia_device {
    IWiaDevMgr2 *dev_manager;
    IWiaItem2 *device;
};

enum wia_src_type {
    WIA_SRC_AUTO = 0,
    WIA_SRC_FLATBED,
    WIA_SRC_FEEDER,
};

struct wia_source {
    wia_src_type type;
    struct wia_device *dev;
    IWiaItem2 *source;
};

static const int g_possible_connect_status[] = {
    WIA_DEVICE_NOT_CONNECTED,
    WIA_DEVICE_CONNECTED,
    -1,
};

static const int g_possible_access_rights[] = {
    WIA_ITEM_READ,
    WIA_ITEM_WRITE,
    WIA_ITEM_CAN_BE_DELETED,
    WIA_ITEM_RD,
    WIA_ITEM_RWD,
    -1,
};

static const int g_possible_compression[] = {
    WIA_COMPRESSION_NONE,
    WIA_COMPRESSION_AUTO,
    WIA_COMPRESSION_BI_RLE4,
    WIA_COMPRESSION_BI_RLE8,
    WIA_COMPRESSION_G3,
    WIA_COMPRESSION_G4,
    WIA_COMPRESSION_JPEG,
    WIA_COMPRESSION_JBIG,
    WIA_COMPRESSION_JPEG2K,
    WIA_COMPRESSION_PNG,
    -1,
};

static const int g_possible_datatype[] = {
    WIA_DATA_AUTO,
    WIA_DATA_COLOR,
    WIA_DATA_COLOR_DITHER,
    WIA_DATA_COLOR_THRESHOLD,
    WIA_DATA_DITHER,
    WIA_DATA_GRAYSCALE,
    WIA_DATA_THRESHOLD,
    WIA_DATA_RAW_BGR,
    WIA_DATA_RAW_CMY,
    WIA_DATA_RAW_CMYK,
    WIA_DATA_RAW_RGB,
    WIA_DATA_RAW_YUV,
    WIA_DATA_RAW_YUVK,
    -1,
};

static const CLSID g_possible_format[] = {
    WiaImgFmt_BMP,
    WiaImgFmt_CIFF,
    WiaImgFmt_EXIF,
    WiaImgFmt_FLASHPIX,
    WiaImgFmt_GIF,
    WiaImgFmt_ICO,
    WiaImgFmt_JBIG,
    WiaImgFmt_JPEG,
    WiaImgFmt_JPEG2K,
    WiaImgFmt_JPEG2KX,
    WiaImgFmt_MEMORYBMP,
    WiaImgFmt_PDFA,
    WiaImgFmt_PHOTOCD,
    WiaImgFmt_PICT,
    WiaImgFmt_PNG,
    WiaImgFmt_RAW,
    WiaImgFmt_RAWRGB,
    WiaImgFmt_TIFF,
    NULL,
};

static const CLSID g_possible_item_category[] = {
    WIA_CATEGORY_ROOT,
    WIA_CATEGORY_FLATBED,
    WIA_CATEGORY_FEEDER,
    WIA_CATEGORY_FEEDER_FRONT,
    WIA_CATEGORY_FEEDER_BACK,
    WIA_CATEGORY_FILM,
    WIA_CATEGORY_FOLDER,
    WIA_CATEGORY_FINISHED_FILE,
    NULL,
};

static const int g_possible_item_flags[] = {
    WiaItemTypeAnalyze,
    WiaItemTypeAudio,
    WiaItemTypeBurst,
    WiaItemTypeDeleted,
    WiaItemTypeDocument,
    WiaItemTypeDevice,
    WiaItemTypeDisconnected,
    WiaItemTypeFile,
    WiaItemTypeFolder,
    WiaItemTypeFree,
    WiaItemTypeGenerated,
    WiaItemTypeHasAttachments,
    WiaItemTypeHPanorama,
    WiaItemTypeImage,
    WiaItemTypeProgrammableDataSource,
    WiaItemTypeRoot,
    WiaItemTypeStorage,
    WiaItemTypeTransfer,
    // WiaItemTypeTwainCapabilityPassThrough, // Jflesch> Doesn't exist ?
    WiaItemTypeVideo,
    WiaItemTypeVPanorama,
    -1,
};

static const int g_possible_planar[] = {
    WIA_PACKED_PIXEL,
    WIA_PLANAR,
    -1,
};

static const int g_possible_suppress_property_page[] = {
    WIA_PROPPAGE_CAMERA_ITEM_GENERAL,
    WIA_PROPPAGE_SCANNER_ITEM_GENERAL,
    -1,
};

static const int g_possible_tymed[] = {
    TYMED_CALLBACK,
    TYMED_MULTIPAGE_CALLBACK,
    TYMED_FILE,
    TYMED_MULTIPAGE_FILE,
    -1,
};

static const int g_possible_dev_type[] = {
    StiDeviceTypeDefault,
    StiDeviceTypeScanner,
    StiDeviceTypeDigitalCamera,
    StiDeviceTypeStreamingVideo,
    -1,
};

static const int g_possible_hw_config[] = {
    1, // Generic WDM device
    2, // SCSI device
    4, // USB device
    8, // Serial device
    16, // Parallel device
    -1,
};

static const int g_possible_document_handling_capabilities[] = {
    AUTO_SOURCE,
    ADVANCED_DUP,
    DETECT_FILM_TPA,
    DETECT_STOR,
    FILM_TPA,
    STOR,
    DETECT_FEED,
    DETECT_FLAT,
    DETECT_SCAN,
    DUP,
    FEED,
    FLAT,
    DETECT_DUP,
    DETECT_DUP_AVAIL,
    DETECT_FEED_AVAIL,
    -1,
};

static const int g_possible_document_handling_select[] = {
    FEEDER,
    FLATBED,
    DUPLEX,
    AUTO_ADVANCE,
    FRONT_FIRST,
    BACK_FIRST,
    FRONT_ONLY,
    BACK_ONLY,
    NEXT_PAGE,
    PREFEED,
    -1,
};

static const int g_possible_document_handling_status[] = {
    FEED_READY,
    FLAT_READY,
    DUP_READY,
    FLAT_COVER_UP,
    PATH_COVER_UP,
    PAPER_JAM,
    FILM_TPA_READY,
    STORAGE_READY,
    STORAGE_FULL,
    MULTIPLE_FEED,
    DEVICE_ATTENTION,
    LAMP_ERR,
    -1,
};

static const int g_possible_horizontal_bed_registration[] = {
    LEFT_JUSTIFIED,
    CENTERED,
    RIGHT_JUSTIFIED,
    -1,
};

static const int g_possible_orientation[] = {
    LANDSCAPE,
    PORTRAIT,
    ROT180,
    ROT270,
    -1,
};

static const int g_possible_page_size[] = {
    WIA_PAGE_A4,
    WIA_PAGE_CUSTOM, // see WIA_DPS_PAGE_HEIGHT and WIA_DPS_PAGE_WIDTH
    WIA_PAGE_LETTER,
    WIA_PAGE_USLEGAL,
    WIA_PAGE_USLETTER,
    WIA_PAGE_USLEDGER,
    WIA_PAGE_USSTATEMENT,
    WIA_PAGE_BUSINESSCARD,
    WIA_PAGE_ISO_A0,
    WIA_PAGE_ISO_A1,
    WIA_PAGE_ISO_A2,
    WIA_PAGE_ISO_A3,
    WIA_PAGE_ISO_A4,
    WIA_PAGE_ISO_A5,
    WIA_PAGE_ISO_A6,
    WIA_PAGE_ISO_A7,
    WIA_PAGE_ISO_A8,
    WIA_PAGE_ISO_A9,
    WIA_PAGE_ISO_A10,
    WIA_PAGE_ISO_B0,
    WIA_PAGE_ISO_B1,
    WIA_PAGE_ISO_B2,
    WIA_PAGE_ISO_B3,
    WIA_PAGE_ISO_B4,
    WIA_PAGE_ISO_B5,
    WIA_PAGE_ISO_B6,
    WIA_PAGE_ISO_B7,
    WIA_PAGE_ISO_B8,
    WIA_PAGE_ISO_B9,
    WIA_PAGE_ISO_B10,
    WIA_PAGE_ISO_C0,
    WIA_PAGE_ISO_C1,
    WIA_PAGE_ISO_C2,
    WIA_PAGE_ISO_C3,
    WIA_PAGE_ISO_C4,
    WIA_PAGE_ISO_C5,
    WIA_PAGE_ISO_C6,
    WIA_PAGE_ISO_C7,
    WIA_PAGE_ISO_C8,
    WIA_PAGE_ISO_C9,
    WIA_PAGE_ISO_C10,
    WIA_PAGE_JIS_B0,
    WIA_PAGE_JIS_B1,
    WIA_PAGE_JIS_B2,
    WIA_PAGE_JIS_B3,
    WIA_PAGE_JIS_B4,
    WIA_PAGE_JIS_B5,
    WIA_PAGE_JIS_B6,
    WIA_PAGE_JIS_B7,
    WIA_PAGE_JIS_B8,
    WIA_PAGE_JIS_B9,
    WIA_PAGE_JIS_B10,
    WIA_PAGE_JIS_2A,
    WIA_PAGE_JIS_4A,
    WIA_PAGE_DIN_2B,
    WIA_PAGE_DIN_2B,
    WIA_PAGE_DIN_4B,
    WIA_PAGE_AUTO,
    WIA_PAGE_CUSTOM_BASE,
    -1,
};

static const int g_possible_preview[] = {
    WIA_FINAL_SCAN,
    WIA_PREVIEW_SCAN,
    -1,
};

static const int g_possible_sheet_feeder_registration[] = {
    LEFT_JUSTIFIED,
    CENTERED,
    RIGHT_JUSTIFIED,
    -1,
};

static const int g_possible_show_preview_control[] = {
    WIA_SHOW_PREVIEW_CONTROL,
    WIA_DONT_SHOW_PREVIEW_CONTROL,
    -1,
};

static const int g_possible_vertical_bed_registration[] = {
    TOP_JUSTIFIED,
    CENTERED,
    BOTTOM_JUSTIFIED,
    -1,
};

static const int g_possible_auto_deskew[] = {
    WIA_AUTO_DESKEW_ON,
    WIA_AUTO_DESKEW_OFF,
    -1,
};

static const int g_possible_intent[] = {
    WIA_INTENT_NONE,
    WIA_INTENT_IMAGE_TYPE_COLOR,
    WIA_INTENT_IMAGE_TYPE_GRAYSCALE,
    WIA_INTENT_IMAGE_TYPE_TEXT,
    WIA_INTENT_IMAGE_TYPE_MASK,
    WIA_INTENT_MINIMIZE_SIZE,
    WIA_INTENT_MAXIMIZE_QUALITY,
    WIA_INTENT_SIZE_MASK,
    WIA_INTENT_BEST_PREVIEW,
    -1,
};

static const int g_possible_film_scan_mode[] = {
    WIA_FILM_COLOR_SLIDE,
    WIA_FILM_COLOR_NEGATIVE,
    WIA_FILM_BW_NEGATIVE,
    -1,
};

static const int g_possible_lamp[] = {
    WIA_LAMP_ON,
    WIA_LAMP_OFF,
    -1,
};

static const int g_possible_photometric_interp[] = {
    WIA_PHOTO_WHITE_0, // WHITE is 0, and BLACK is 1
    WIA_PHOTO_WHITE_1, // WHITE is 1, and BLACK is 0
    -1,
};

static const int g_possible_preview_type[] = {
    WIA_ADVANCED_PREVIEW,
    WIA_BASIC_PREVIEW,
    -1,
};

static const int g_possible_rotation[] = {
    PORTRAIT,
    LANDSCAPE,
    ROT180,
    ROT270,
    -1,
};

static const int g_possible_segmentation[] = {
    WIA_USE_SEGMENTATION_FILTER,
    WIA_DONT_USE_SEGMENTATION_FILTER,
    -1,
};

struct wia_property {
    PROPID id;
    const char *name;
    int rw;
    const void *possible_values;
};

static const struct wia_property g_all_properties[] =
{
    {
        WIA_DPA_CONNECT_STATUS, "connect_status", 0, g_possible_connect_status,
    },
    {
        WIA_DPA_DEVICE_TIME, "device_time", 0, NULL,
    },
    {
        WIA_DPA_FIRMWARE_VERSION, "firmware version", 0, NULL,
    },
    {
        WIA_IPA_ACCESS_RIGHTS, "access_rights", 1,
        g_possible_access_rights,
    },
    {
        WIA_IPA_BITS_PER_CHANNEL, "bits_per_channel", 0, NULL,
    },
    {
        WIA_IPA_BUFFER_SIZE, "buffer_size", 0, NULL,
    },
    {
        WIA_IPA_BYTES_PER_LINE, "bytes_per_line", 0, NULL,
    },
    {
        WIA_IPA_CHANNELS_PER_PIXEL, "channels_per_pixel", 0, NULL,
    },
    {
        WIA_IPA_COLOR_PROFILE, "color_profile", 0, NULL,
    },
    {
        WIA_IPA_COMPRESSION, "compression", 1, g_possible_compression,
    },
    {
        WIA_IPA_DATATYPE, "datatype", 1, g_possible_datatype,
    },
    {
        WIA_IPA_DEPTH, "depth", 1, NULL,
    },
    {
        WIA_IPA_FILENAME_EXTENSION, "filename_extension", 0, NULL,
    },
    {
        WIA_IPA_FORMAT, "format", 1, g_possible_format,
    },
    {
        WIA_IPA_FULL_ITEM_NAME, "full_item_name", 0, NULL,
    },
    {
        WIA_IPA_GAMMA_CURVES, "gamma_curves", 0, NULL,
    },
    {
        WIA_IPA_ICM_PROFILE_NAME, "icm_profile_name", 0, NULL,
    },
    {
        WIA_IPA_ITEM_CATEGORY, "item_category", 0, g_possible_item_category,
    },
    {
        WIA_IPA_ITEM_FLAGS, "item_flags", 0, g_possible_item_flags,
    },
    {
        WIA_IPA_ITEM_NAME, "item_name", 0, NULL,
    },
    {
        WIA_IPA_ITEM_SIZE, "item_size", 0, NULL,
    },
    {
        WIA_IPA_ITEM_TIME, "item_time", 1, NULL,
    },
    {
        WIA_IPA_ITEMS_STORED, "items_stored", 1, NULL,
    },
    {
        WIA_IPA_MIN_BUFFER_SIZE, "buffer_size", 0, NULL,
    },
    {
        WIA_IPA_NUMBER_OF_LINES, "number_of_lines", 0, NULL,
    },
    {
        WIA_IPA_PIXELS_PER_LINE, "pixels_per_line", 0, NULL,
    },
    {
        WIA_IPA_PLANAR, "planar", 1, g_possible_planar,
    },
    {
        WIA_IPA_PREFERRED_FORMAT, "preferred_format", 0, g_possible_format,
    },
    {
        WIA_IPA_PROP_STREAM_COMPAT_ID, "prop_stream_compat_id", 0, NULL,
    },
    {
        WIA_IPA_RAW_BITS_PER_CHANNEL, "raw_bits_per_channel", 0, NULL,
    },
    {
        WIA_IPA_REGION_TYPE, "region_type", 0, NULL,
    },
    {
        WIA_IPA_SUPPRESS_PROPERTY_PAGE, "suppress_property_page", 0, g_possible_suppress_property_page,
    },
    {
        WIA_IPA_TYMED, "tymed", 1, g_possible_tymed,
    },
    {
        WIA_IPA_UPLOAD_ITEM_SIZE, "upload_item_size", 1, NULL,
    },
    {
        WIA_DIP_DEV_ID, "dev_id", 0, NULL,
    },
    {
        WIA_DIP_VEND_DESC, "vend_desc", 0, NULL,
    },
    {
        WIA_DIP_DEV_DESC, "dev_desc", 0, NULL,
    },
    {
        WIA_DIP_DEV_TYPE, "dev_type", 0, g_possible_dev_type,
    },
    {
        WIA_DIP_PORT_NAME, "port_name", 0, NULL,
    },
    {
        WIA_DIP_DEV_NAME, "dev_name", 0, NULL,
    },
    {
        WIA_DIP_SERVER_NAME, "server_name", 0, NULL,
    },
    {
        WIA_DIP_REMOTE_DEV_ID, "remote_dev_id", 0, NULL,
    },
    {
        WIA_DIP_UI_CLSID, "ui_clsid", 0, NULL,
    },
    {
        WIA_DIP_HW_CONFIG, "hw_config", 0, g_possible_hw_config,
    },
    {
        WIA_DIP_BAUDRATE, "baudrate", 0, NULL,
    },
    {
        WIA_DIP_STI_GEN_CAPABILITIES, "sti_gen_capabilities", 0, NULL,
    },
    {
        WIA_DIP_WIA_VERSION, "wia_version", 0, NULL,
    },
    {
        WIA_DIP_DRIVER_VERSION, "driver_version", 0, NULL,
    },
    {
        WIA_DIP_PNP_ID, "pnp_id", 0, NULL,
    },
    {
        WIA_DIP_STI_DRIVER_VERSION, "sti_driver_version", 0, NULL,
    },
    {
        WIA_DPS_DEVICE_ID, "device_id", 0, NULL,
    },
    {
        WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES, "document_handling_capabilities", 0, g_possible_document_handling_capabilities,
    },
    {
        WIA_DPS_DOCUMENT_HANDLING_SELECT, "document_handling_select", 1, g_possible_document_handling_select,
    },
    {
        WIA_DPS_DOCUMENT_HANDLING_STATUS, "document_handling_status", 0, g_possible_document_handling_status,
    },
    {
        WIA_DPS_ENDORSER_CHARACTERS, "endorser_characters", 0, NULL,
    },
    {
        WIA_DPS_ENDORSER_STRING, "endorser_string", 1, NULL,
    },
    {
        WIA_DPS_GLOBAL_IDENTITY, "global_identity", 0, NULL,
    },
    {
        WIA_DPS_HORIZONTAL_BED_REGISTRATION, "horizontal_bed_registration", 0, g_possible_horizontal_bed_registration,
    },
    {
        WIA_DPS_HORIZONTAL_BED_SIZE, "horizontal_bed_size", 0, NULL,
    },
    {
        WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE, "horizontal_sheet_feed_size", 0, NULL,
    },
    {
        WIA_DPS_MAX_SCAN_TIME, "max_scan_time", 0, NULL,
    },
    {
        WIA_DPS_MIN_HORIZONTAL_SHEET_FEED_SIZE, "min_horizontal_sheet_feed_size", 0, NULL,
    },
    {
        WIA_DPS_MIN_VERTICAL_SHEET_FEED_SIZE, "min_vertical_sheet_feed_size", 0, NULL,
    },
    {
        WIA_DPS_OPTICAL_XRES, "optical_xres", 0, NULL,
    },
    {
        WIA_DPS_OPTICAL_YRES, "optical_yres", 0, NULL,
    },
    // TODO(JFlesch): Visual C++ says WIA_DPS_ORIENTATION doesn't exist ?!
    //{
    //    WIA_DPS_ORIENTATION, "orientation", 1, g_possible_orientation,
    //},
    {
        WIA_DPS_PAD_COLOR, "pad_color", 0, NULL,
    },
    {
        WIA_DPS_PAGE_HEIGHT, "page_height", 0, NULL,
    },
    {
        WIA_DPS_PAGE_SIZE, "page_size", 1, g_possible_page_size,
    },
    {
        WIA_DPS_PAGE_WIDTH, "page_width", 0, NULL,
    },
    {
        WIA_DPS_PAGES, "pages", 1, NULL,
    },
    {
        WIA_DPS_PLATEN_COLOR, "platen_color", 0, NULL,
    },
    {
        WIA_DPS_PREVIEW, "preview", 1, g_possible_preview,
    },
    {
        WIA_DPS_SCAN_AHEAD_PAGES, "scan_ahead_pages", 1, NULL,
    },
    {
        WIA_DPS_SCAN_AVAILABLE_ITEM, "scan_available_item", 1, NULL,
    },
    {
        WIA_DPS_SERVICE_ID, "service_id", 0, NULL,
    },
    {
        WIA_DPS_SHEET_FEEDER_REGISTRATION, "sheet_feeder_registration", 0, g_possible_sheet_feeder_registration,
    },
    {
        WIA_DPS_SHOW_PREVIEW_CONTROL, "show_preview_control", 0, g_possible_show_preview_control,
    },
    {
        WIA_DPS_USER_NAME, "user_name", 0, NULL,
    },
    {
        WIA_DPS_VERTICAL_BED_REGISTRATION, "vertical_bed_registration", 0, g_possible_vertical_bed_registration,
    },
    {
        WIA_DPS_VERTICAL_BED_SIZE, "vertical_bed_size", 0, NULL,
    },
    {
        WIA_DPS_VERTICAL_SHEET_FEED_SIZE, "vertical_sheet_feed_size", 0, NULL,
    },
    {
        WIA_IPS_AUTO_DESKEW, "auto_deskew", 1, g_possible_auto_deskew,
    },
    {
        WIA_IPS_BRIGHTNESS, "brightness", 1, NULL,
    },
    {
        WIA_IPS_CONTRAST, "contrast", 1, NULL,
    },
    {
        WIA_IPS_CUR_INTENT, "current_intent", 1, g_possible_intent,
    },
    {
        WIA_IPS_DESKEW_X, "deskew_x", 1, NULL,
    },
    {
        WIA_IPS_DESKEW_Y, "deskew_y", 1, NULL,
    },
    {
        WIA_IPS_DOCUMENT_HANDLING_SELECT, "document_handling_select", 1, g_possible_document_handling_select,
    },
    {
        WIA_IPS_FILM_NODE_NAME, "film_node_name", 0, NULL,
    },
    {
        WIA_IPS_FILM_SCAN_MODE, "file_scan_mode", 1, g_possible_film_scan_mode,
    },
    {
        WIA_IPS_INVERT, "invert", 0, NULL,
    },
    {
        WIA_IPA_ITEMS_STORED, "items_stored", 0, NULL,
    },
    {
        WIA_IPS_LAMP, "lamp", 1, g_possible_lamp,
    },
    {
        WIA_IPS_LAMP_AUTO_OFF, "lamp_auto_off", 1, NULL,
    },
    {
        WIA_IPS_MAX_HORIZONTAL_SIZE, "max_horizontal_size", 0, NULL,
    },
    {
        WIA_IPS_MAX_VERTICAL_SIZE, "max_vertical_size", 0, NULL,
    },
    {
        WIA_IPS_MIN_HORIZONTAL_SIZE, "min_horizontal_size", 0, NULL,
    },
    {
        WIA_IPS_MIN_VERTICAL_SIZE, "min_vertical_size", 0, NULL,
    },
    {
        WIA_IPS_MIRROR, "mirror", 0, NULL,
    },
    {
        WIA_IPS_OPTICAL_XRES, "optical_xres", 0, NULL,
    },
    {
        WIA_IPS_OPTICAL_YRES, "optical_yres", 0, NULL,
    },
    {
        WIA_IPS_ORIENTATION, "orientation", 1, g_possible_orientation,
    },
    {
        WIA_IPS_PAGE_SIZE, "page_size", 1, g_possible_page_size,
    },
    {
        WIA_IPS_PAGE_HEIGHT, "page_height", 0, NULL,
    },
    {
        WIA_IPS_PAGE_WIDTH, "page_width", 0, NULL,
    },
    {
        WIA_IPS_PAGES, "pages", 1, NULL,
    },
    {
        WIA_IPS_PHOTOMETRIC_INTERP, "photometric_interp", 1, g_possible_photometric_interp,
    },
    {
        WIA_IPS_PREVIEW, "preview", 1, g_possible_preview,
    },
    {
        WIA_IPS_PREVIEW_TYPE, "preview_type", 0, g_possible_preview_type,
    },
    {
        WIA_IPS_ROTATION, "rotation", 1, g_possible_rotation,
    },
    {
        WIA_IPS_SEGMENTATION, "segmentation", 0, g_possible_segmentation,
    },
    {
        WIA_IPS_SHEET_FEEDER_REGISTRATION, "sheet_feeder_registration", 0, g_possible_sheet_feeder_registration,
    },
    {
        WIA_IPS_SHOW_PREVIEW_CONTROL, "show_preview_control", 0, g_possible_show_preview_control,
    },
    {
        WIA_IPS_SUPPORTS_CHILD_ITEM_CREATION, "supportes_child_item_creation", 0, NULL,
    },
    {
        WIA_IPS_THRESHOLD, "threshold", 1, NULL,
    },
    {
        WIA_IPS_TRANSFER_CAPABILITIES, "transfer_capabilities", 0, NULL,
    },
    {
        WIA_IPA_UPLOAD_ITEM_SIZE, "upload_item_size", 1, NULL,
    },
    {
        WIA_IPS_WARM_UP_TIME, "warm_up_time", 0, NULL,
    },
    {
        WIA_IPS_XEXTENT, "xextent", 1, NULL,
    },
    {
        WIA_IPS_XPOS, "xpos", 1, NULL,
    },
    {
        WIA_IPS_XRES, "xres", 1, NULL,
    },
    {
        WIA_IPS_XSCALING, "xscaling", 1, NULL,
    },
    {
        WIA_IPS_YEXTENT, "yextent", 1, NULL,
    },
    {
        WIA_IPS_YPOS, "ypos", 1, NULL,
    },
    {
        WIA_IPS_YRES, "yres", 1, NULL,
    },
    {
        WIA_IPS_YSCALING, "yscaling", 1, NULL,
    }
};

static PyObject *init(PyObject *, PyObject* args)
{
    HRESULT hr;

	if (!PyArg_ParseTuple(args, "")) {
		return NULL;
	}

    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        WIA_WARNING("Pyinsane: WARNING: CoInitialize() failed !");
        Py_RETURN_NONE;
    }

	Py_RETURN_NONE;
}


static HRESULT get_device_basic_infos(IWiaPropertyStorage *properties,
    PyObject **out_tuple)
{
    PyObject *devid, *devname;
    PROPSPEC input[3] = {0};
    PROPVARIANT output[3] = {0};
    HRESULT hr;

    *out_tuple = NULL;

    input[0].ulKind = PRSPEC_PROPID;
    input[0].propid = WIA_DIP_DEV_ID;
    input[1].ulKind = PRSPEC_PROPID;
    input[1].propid = WIA_DIP_DEV_NAME;
    input[2].ulKind = PRSPEC_PROPID;
    input[2].propid = WIA_DIP_DEV_TYPE;

    hr = properties->ReadMultiple(3 /* nb_properties */, input, output);
    if (FAILED(hr)) {
        WIA_WARNING("Pyinsane: WiaPropertyStorage->ReadMultiple() failed");
        return hr;
    }

    assert(output[0].vt == VT_BSTR);
    assert(output[1].vt == VT_BSTR);
    assert(output[2].vt == VT_I4);

    if (GET_STIDEVICE_TYPE(output[2].lVal) != StiDeviceTypeScanner) {
        *out_tuple = NULL;
        return S_OK;
    }

    devid = PyUnicode_FromWideChar(output[0].bstrVal, -1);
    devname = PyUnicode_FromWideChar(output[1].bstrVal, -1);

    *out_tuple = PyTuple_Pack(2, devid, devname);

    FreePropVariantArray(2, output);

    return S_OK;
}


static PyObject *get_devices(PyObject *, PyObject* args)
{
    HRESULT hr;
    CComPtr<IWiaDevMgr2> wia_dev_manager;
    CComPtr<IEnumWIA_DEV_INFO> wia_dev_info_enum;
    unsigned long nb_devices;
    PyObject *dev_infos;
    PyObject *all_devs;

	if (!PyArg_ParseTuple(args, "")) {
		return NULL;
	}

    // Create a connection to the local WIA device manager
    hr = wia_dev_manager.CoCreateInstance(CLSID_WiaDevMgr2);
    if (FAILED(hr)) {
        WIA_WARNING("Pyinsane: WARNING: CoCreateInstance failed");
        Py_RETURN_NONE;
    }

    hr = wia_dev_manager->EnumDeviceInfo(WIA_DEVINFO_ENUM_LOCAL, &wia_dev_info_enum);
    if (FAILED(hr)) {
        WIA_WARNING("Pyinsane: WARNING: WiaDevMgr->EnumDviceInfo() failed");
        Py_RETURN_NONE;
    }

    // Get the numeber of WIA devices

    hr = wia_dev_info_enum->GetCount(&nb_devices);
    if (FAILED(hr)) {
        WIA_WARNING("PyInsane: WARNING: GetCount() failed !");
        Py_RETURN_NONE;
    }

    all_devs = PyList_New(0);

    while (hr == S_OK) {
        IWiaPropertyStorage *properties = NULL;
        hr = wia_dev_info_enum->Next(1, &properties, NULL);
        if (hr != S_OK || properties == NULL)
            break;

        hr = get_device_basic_infos(properties, &dev_infos);
        if (FAILED(hr)) {
            Py_RETURN_NONE;
        }
        if (dev_infos == NULL) {
            // not a scanner
            continue;
        }

        properties->Release();

        PyList_Append(all_devs, dev_infos);
    }

    // wia_dev_info_enum->Release(); // TODO(Jflesch) ?
    return all_devs;
}

static void free_device(PyObject *device)
{
    struct wia_device *wia_dev;

    wia_dev = (struct wia_device *)PyCapsule_GetPointer(device, WIA_PYCAPSULE_DEV_NAME);
    // TODO
    free(wia_dev);
}

static PyObject *open_device(PyObject *, PyObject *args)
{
    char *devid;
    CComPtr<IWiaDevMgr2> wia_dev_manager;
    struct wia_device *dev;
    BSTR bstr_devid;
    HRESULT hr;
    USES_CONVERSION;

    if (!PyArg_ParseTuple(args, "s", &devid)) {
        return NULL;
    }

    hr = wia_dev_manager.CoCreateInstance(CLSID_WiaDevMgr2);
    if (FAILED(hr)) {
        WIA_WARNING("Pyinsane: WARNING: CoCreateInstance failed");
        Py_RETURN_NONE;
    }

    dev = (struct wia_device *)calloc(1, sizeof(struct wia_device));
    dev->dev_manager = wia_dev_manager;

    bstr_devid = SysAllocString(A2W(devid)); // TODO(Jflesch): Does any of this allocate anything ? oO
    hr = wia_dev_manager->CreateDevice(0, bstr_devid, &dev->device);
    if (FAILED(hr)) {
        WIA_WARNING("Pyinsane: WARNING: WiaDevMgr->CreateDevice() failed");
        free(dev);
        Py_RETURN_NONE;
    }

    return PyCapsule_New(dev, WIA_PYCAPSULE_DEV_NAME, free_device);
}

static void free_source(PyObject *source)
{
    struct wia_source *wia_src;

    wia_src = (struct wia_source *)PyCapsule_GetPointer(source, WIA_PYCAPSULE_DEV_NAME);
    // TODO
    free(wia_src);
}

static PyObject *get_sources(PyObject *, PyObject *args)
{
    struct wia_device *dev;
    IEnumWiaItem2 *enum_item;
    IWiaItem2 *child;
    PyObject *source_name;
    PyObject *capsule;
    PyObject *tuple;
    PyObject *all_sources;
    struct wia_source *source;
    PROPSPEC input[2] = {0};
    PROPVARIANT output[2] = {0};
    HRESULT hr;

    input[0].ulKind = PRSPEC_PROPID;
    input[0].propid = WIA_IPA_FULL_ITEM_NAME;
    input[1].ulKind = PRSPEC_PROPID;
    input[1].propid = WIA_IPA_ITEM_CATEGORY;

    if (!PyArg_ParseTuple(args, "O", &capsule)) {
        WIA_WARNING("Pyinsane: get_sources(): Invalid args");
        return NULL;
    }
    if (!PyCapsule_CheckExact(capsule)) {
        WIA_WARNING("Pyinsane: WARNING: get_sources(): invalid argument type (not a pycapsule)");
        Py_RETURN_NONE;
    }

    if ((dev = (struct wia_device *)PyCapsule_GetPointer(capsule, WIA_PYCAPSULE_DEV_NAME)) == NULL) {
        WIA_WARNING("Pyinsane: WARNING: get_sources(): invalid argument type");
        Py_RETURN_NONE;
    }

    all_sources = PyList_New(0);

    hr = dev->device->EnumChildItems(NULL, &enum_item);
    while(hr == S_OK) {
        hr = enum_item->Next(1, &child, NULL);
        if (hr != S_OK) {
            continue;
        }

        CComQIPtr<IWiaPropertyStorage> child_properties(child);

        source = (struct wia_source *)calloc(2, sizeof(struct wia_source));
        source->dev = dev;
        source->source = child;

        hr = child_properties->ReadMultiple(2 /* nb_properties */, input, output);
        if (FAILED(hr)) {
            WIA_WARNING("Pyinsane: WiaPropertyStorage->ReadMultiple() failed");
            child->Release();
            continue;
        }

        assert(output[0].vt == VT_BSTR);
        assert(output[1].vt == VT_CLSID);

        if (*output[1].puuid == WIA_CATEGORY_FINISHED_FILE
                    || *output[1].puuid == WIA_CATEGORY_FOLDER
                    || *output[1].puuid == WIA_CATEGORY_ROOT) {
                free(source);
                continue;
        } else if (*output[1].puuid == WIA_CATEGORY_AUTO) {
                source->type = WIA_SRC_AUTO;
        } else if (*output[1].puuid == WIA_CATEGORY_FEEDER
                    || *output[1].puuid == WIA_CATEGORY_FEEDER_BACK
                    || *output[1].puuid == WIA_CATEGORY_FEEDER_FRONT) {
                source->type = WIA_SRC_FEEDER;
        } else {
            source->type = WIA_SRC_FLATBED;
        }

        source_name = PyUnicode_FromWideChar(output[0].bstrVal, -1);
        capsule = PyCapsule_New(source, WIA_PYCAPSULE_SRC_NAME, free_source);
        tuple = PyTuple_Pack(2, source_name, capsule);
        PyList_Append(all_sources, tuple);
    }

    return all_sources;
}

static PyObject *exit(PyObject *, PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
		return NULL;
	}

    CoUninitialize();

	Py_RETURN_NONE;
}


static PyMethodDef rawapi_methods[] = {
	{"init", init, METH_VARARGS, NULL},
	{"get_devices", get_devices, METH_VARARGS, NULL},
	{"get_sources", get_sources, METH_VARARGS, NULL},
	{"open", open_device, METH_VARARGS, NULL},
	{"exit", exit, METH_VARARGS, NULL},
	{NULL, NULL, 0, NULL},
};

#if PY_VERSION_HEX < 0x03000000

PyMODINIT_FUNC
init_rawapi(void)
{
    Py_InitModule("_rawapi", rawapi_methods);
}

#else

static struct PyModuleDef rawapi_module = {
	PyModuleDef_HEAD_INIT,
	"_rawapi",
	NULL /* doc */,
	-1,
	rawapi_methods,
};

PyMODINIT_FUNC PyInit__rawapi(void)
{
	return PyModule_Create(&rawapi_module);
}
#endif
