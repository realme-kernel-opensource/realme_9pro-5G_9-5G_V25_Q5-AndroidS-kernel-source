{
    .need_standby_mode = 1,
    .flashprobeinfo =
	{
		.flash_name = "aw36515",
		.slave_write_address = 0xc6,
		.flash_id_address = 0x0C,
		.flash_id = 0x02,
		.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
	},
    .cci_client =
	{
        .cci_i2c_master = MASTER_0,
        .i2c_freq_mode = I2C_STANDARD_MODE,
        .sid = 0xc6 >> 1,
	},
	.flashinitsettings =
	{
		.reg_setting =
		{

			{.reg_addr = 0x07, .reg_data = 0x09, .delay = 0x00, .data_mask = 0x00}, \
			{.reg_addr = 0x01, .reg_data = 0x00, .delay = 0x00, .data_mask = 0x00}, \
			{.reg_addr = 0x03, .reg_data = 0x8c, .delay = 0x00, .data_mask = 0x00}, \
			{.reg_addr = 0x05, .reg_data = 0x20, .delay = 0x00, .data_mask = 0x00}, \
			{.reg_addr = 0x08, .reg_data = 0x1F, .delay = 0x00, .data_mask = 0x00}, \
			{.reg_addr = 0x01, .reg_data = 0x01, .delay = 0x00, .data_mask = 0x00}, \
		},
		.size = 6,
		.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.delay = 1,
	},
	.flashhighsettings =
	{
		.reg_setting =
		{
			{.reg_addr = 0x07, .reg_data = 0x09, .delay = 0x00, .data_mask = 0x00}, \
			{.reg_addr = 0x01, .reg_data = 0x0D, .delay = 0x00, .data_mask = 0x00}, \
		},
		.size = 2,
		.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.delay = 1,
	},
	.flashlowsettings =
	{
		.reg_setting =
		{
			{.reg_addr = 0x07, .reg_data = 0x09, .delay = 0x00, .data_mask = 0x00}, \
			{.reg_addr = 0x01, .reg_data = 0x09, .delay = 0x00, .data_mask = 0x00}, \
		},
		.size = 2,
		.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.delay = 1,
	},
	.flashoffsettings =
	{
		.reg_setting =
		{
			{.reg_addr = 0x01, .reg_data = 0x00, .delay = 0x00, .data_mask = 0x00}, \
			//into standby mode
			{.reg_addr = 0x07, .reg_data = 0x04, .delay = 0x00, .data_mask = 0x00}, \
		},
		.size = 2,
		.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.delay = 1,
	},
	.flashpowerupsetting =
	{
		.seq_type = SENSOR_CUSTOM_GPIO1,
		.config_val = 1,
		.delay = 5,
		.size = 1,
	},
	.flashpowerdownsetting =
	{
		.seq_type = SENSOR_CUSTOM_GPIO1,
		.config_val = 0,
		.delay = 1,
		.size = 1,
	},
},
