/*
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  Copyright (C) 2015 Intel Corporation
 *
 */

#ifndef __ALSA_TOPOLOGY_H
#define __ALSA_TOPOLOGY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup topology Topology Interface
 * \{
 */

/*! \page topology ALSA Topology Interface
 *
 * The topology interface allows developers to define DSP topologies in a text
 * file format and to convert the text topology to a binary topology
 * representation that can be understood by the kernel. The topology core
 * currently recognises the following object types :-
 *
 *  * Controls (mixer, enumerated and byte) including TLV data.
 *  * PCMs (FE and BE configurations and capabilities)
 *  * DAPM widgets
 *  * DAPM graph elements.
 *  * Private data for each object type.
 *  * Manifest (containing count of each object type)
 *
 * <h3>Topology File Format</h3>
 *
 * The topology text format uses the standard ALSA configuration file format to
 * describe each topology object type. This allows topology objects to include
 * other topology objects as part of their definition. i.e. a TLV data object
 * can be shared amongst many control objects that use the same TLV data.
 *
 *
 * <h4>Controls</h4>
 * Topology audio controls can belong to three different types :-
 *   * Mixer control
 *   * Enumerated control
 *   * Byte control
 *
 * Each control type can contain TLV data, private data, operations and also
 * belong to widget objects.<br>
 *
 * <h5>Control Operations</h5>
 * Driver Kcontrol callback info(), get() and put() operations are mapped with
 * the CTL ops section in topology configuration files. The ctl ops section can
 * assign operations using the standard names (listed below) for the standard
 * kcontrol types or use ID numbers (>256) to map to bespoke driver controls.<br>
 *
 * <pre>
 *
 *	ops."ctl" {
 *		info "volsw"
 *		get "257"
 *		put "257"
 *	}
 *
 * </pre>
 *
 * This mapping shows info() using the standard "volsw" info callback whilst
 * the get() and put() are mapped to bespoke driver callbacks. <br>
 *
 * The Standard operations names for control get(), put() and info calls
 * are :-
 *  * volsw
 *  * volsw_sx
 *  * volsw_xr_sx
 *  * enum
 *  * bytes
 *  * enum_value
 *  * range
 *  * strobe
 *
 * <h5>Control TLV Data</h5>
 * Controls can also use TLV data to represent dB information. This can be done
 * by defining a TLV section and using the TLV section within the control.
 * The TLV data for DBScale types are defined as follows :-
 *
 * <pre>
 *	scale {
 *		min "-9000"
 *		step "300"
 *		mute "1"
 *	}
 * </pre>
 *
 * Where the meanings and values for min, step and mute are exactly the same
 * as defined in driver code.
 *
 * <h5>Control Channel Mapping</h5>
 * Controls can also specify which channels they are mapped with. This is useful
 * for userspace as it allows applications to determine the correct control
 * channel for Left and Right etc. Channel maps are defined as follows :-
 *
 * <pre>
 *	channel."name" {
 *		reg "0"
 *		shift "0"
 *	}
 * </pre>
 *
 * The channel map reg is the register offset for the control, shift is the
 * bit shift within the register for the channel and the section name is the
 * channel name and can be one of the following :-
 *
 * <pre>
 *  * mono		# mono stream
 *  * fl 		# front left
 *  * fr		# front right
 *  * rl		# rear left
 *  * rr		# rear right
 *  * fc		# front center
 *  * lfe		# LFE
 *  * sl		# side left
 *  * sr		# side right
 *  * rc		# rear center
 *  * flc		# front left center
 *  * frc		# front right center
 *  * rlc		# rear left center
 *  * rrc		# rear right center
 *  * flw		# front left wide
 *  * frw		# front right wide
 *  * flh		# front left high
 *  * fch		# front center high
 *  * frh		# front right high
 *  * tc		# top center
 *  * tfl		# top front left
 *  * tfr		# top front right
 *  * tfc		# top front center
 *  * trl		# top rear left
 *  * trr		# top rear right
 *  * trc		# top rear center
 *  * tflc		# top front left center
 *  * tfrc		# top front right center
 *  * tsl		# top side left
 *  * tsr		# top side right
 *  * llfe		# left LFE
 *  * rlfe		# right LFE
 *  * bc		# bottom center
 *  * blc		# bottom left center
 *  * brc		# bottom right center
 * </pre>
 *
 *  <h5>Control Private Data</h5>
 * Controls can also have private data. This can be done by defining a private
 * data section and including the section within the control. The private data
 * section is defined as follows :-
 *
 * <pre>
 * SectionData."pdata for EQU1" {
 *	file "/path/to/file"
 *	bytes "0x12,0x34,0x56,0x78"
 *	shorts "0x1122,0x3344,0x5566,0x7788"
 *	words "0xaabbccdd,0x11223344,0x66aa77bb,0xefef1234"
 * };
 * </pre>
 * The file, bytes, shorts and words keywords are all mutually exclusive as
 * the private data should only be taken from one source.  The private data can
 * either be read from a separate file or defined in the topology file using
 * the bytes, shorts or words keywords.
 *
 * <h5>Mixer Controls</h5>
 * A mixer control is defined as a new section that can include channel mapping,
 * TLV data, callback operations and private data. The mixer section also
 * includes a few other config options that are shown here :-
 *
 * <pre>
 * SectionControlMixer."mixer name" {
 *	comment "optional comments"
 *
 *	index "1"			# Index number
 *
 *	channel."name" {		# Channel maps
 *	   ....
 *	}
 *
 *	ops."ctl" {			# Ops callback functions
 *	   ....
 *	}
 *
 *	max "32"			# Max control value
 *	invert "0"			# Whether control values are inverted
 *
 *	tlv "tld_data"			# optional TLV data
 *
 *	data "pdata for mixer1"		# optional private data
 * }
 * </pre>
 *
 * The section name is used to define the mixer name. The index number can be
 * used to identify topology objects groups. This allows driver operations on
 * objects with index number N and can be used to add/remove pipelines of
 * objects whilst other objects are unaffected.
 *
 * <h5>Byte Controls</h5>
 * A byte control is defined as a new section that can include channel mapping,
 * TLV data, callback operations and private data. The bytes section also
 * includes a few other config options that are shown here :-
 *
 * <pre>
 * SectionControlBytes."name" {
 *	comment "optional comments"
 *
 *	index "1"			# Index number
 *
 *	channel."name" {		# Channel maps
 *	   ....
 *	}
 *
 *	ops."ctl" {			# Ops callback functions
 *	   ....
 *	}
 *
 *	base "0"			# Register base
 *	num_regs "16"			# Number of registers
 *	mask "0xff"			# Mask
 *	max "255"			# Maximum value
 *
 *	tlv "tld_data"			# optional TLV data
 *
 *	data "pdata for mixer1"		# optional private data
 * }
 * </pre>
 *
 * <h5>Enumerated Controls</h5>
 * A enumerated control is defined as a new section (like mixer and byte) that
 * can include channel mapping, callback operations, private data and
 * text strings to represent the enumerated control options.<br>
 *
 * The text strings for the enumerated controls are defined in a separate
 * section as follows :-
 *
 * <pre>
 * SectionText."name" {
 *
 *		Values [
 *			"value1"
 *			"value2"
			"value3"
 *		]
 * }
 * </pre>
 *
 * All the enumerated text values are listed in the values list.<br>
 * The enumerated control is similar to the other controls and defined as
 * follows :-
 *
 * <pre>
 * SectionControlMixer."name" {
 *	comment "optional comments"
 *
 *	index "1"			# Index number
 *
 *	texts "EQU1"			# Enumerated text items
 *
 *	channel."name" {		# Channel maps
 *	   ....
 *	}
 *
 *	ops."ctl" {			# Ops callback functions
 *	   ....
 *	}
 *
 *	data "pdata for mixer1"		# optional private data
 * }
 * </pre>
 *
 * <h4>DAPM Graph</h4>
 * DAPM graphs can easily be defined using the topology file. The format is
 * very similar to the DAPM graph kernel format. :-
 *
 * <pre>
 * SectionGraph."dsp" {
 *	index "1"			# Index number
 *
 *	lines [
 *		"sink1, control, source1"
 *		"sink2, , source2"
 *	]
 * }
 * </pre>
 *
 * The lines in the graph are defined as a variable size list of sinks,
 * controls and sources. The control name is optional as some graph lines have
 * no associated controls. The section name can be used to differentiate the
 * graph with other graphs, it's not used by the kernel atm.
 *
 * <h4>DAPM Widgets</h4>
 * DAPM widgets are similar to controls in that they can include many other
 * objects. Widgets can contain private data, mixer controls and enum controls.
 *
 * The following widget types are supported and match the driver types :-
 *
 *  * input
 *  * output
 *  * mux
 *  * mixer
 *  * pga
 *  * out_drv
 *  * adc
 *  * dac
 *  * switch
 *  * pre
 *  * post
 *  * aif_in
 *  * aif_out
 *  * dai_in
 *  * dai_out
 *  * dai_link
 *
 * Widgets are defined as follows :-
 *
 * <pre>
 * SectionWidget."name" {
 *
 *	index "1"			# Index number
 *
 *	type "aif_in"			# Widget type - detailed above
 *
 *	no_pm "true"			# No PM control bit.
 *	reg "20"			# PM bit register offset
 *	shift "0"			# PM bit register shift
 *	invert "1			# PM bit is inverted
 *	subseq "8"			# subsequence number
 *
 *	event_type "1"			# DAPM widget event type
 *	event_flags "1"			# DAPM widget event flags
 *
 *	mixer "name"			# Optional Mixer Control
 *	enum "name"			# Optional Enum Control
 *
 *	data "name"			# optional private data
 * }
 * </pre>
 *
 * The section name is the widget name. The mixer and enum fields are mutually
 * exclusive and used to include controls into the widget. The index and data
 * fields are the same for widgets as they are for controls whilst the other
 * fields map on very closely to the driver widget fields.
 *
 * <h4>PCM Capabilities</h4>
 * Topology can also define the capabilities of FE and BE PCMs. Capabilities
 * can be defined with the following section :-
 *
 * <pre>
 * SectionPCMCapabilities."name" {
 *
 *	formats "S24_LE,S16_LE"		# Supported formats
 *	rate_min "48000"		# Max supported sample rate
 *	rate_max "48000"		# Min supported sample rate
 *	channels_min "2"		# Min number of channels
 *	channels_max "2"		# max number of channels
 * }
 * </pre>
 * The supported formats use the same naming convention as the driver macros.
 * The PCM capabilities name can be referred to and included by BE, PCM and
 * Codec <-> codec topology sections.
 *
 * <h4>PCM Configurations</h4>
 * PCM runtime configurations can be defined for playback and capture stream
 * directions with the following section  :-
 *
 * <pre>
 * SectionPCMConfig."name" {
 *
 *	config."playback" {		# playback config
 *		format "S16_LE"		# playback format
 *		rate "48000"		# playback sample rate
 *		channels "2"		# playback channels
 *		tdm_slot "0xf"		# playback TDM slot
 *	}
 *
 *	config."capture" {		# capture config
 *		format "S16_LE"		# capture format
 *		rate "48000"		# capture sample rate
 *		channels "2"		# capture channels
 *		tdm_slot "0xf"		# capture TDM slot
 *	}
 * }
 * </pre>
 *
 * The supported formats use the same naming convention as the driver macros.
 * The PCM configuration name can be referred to and included by BE, PCM and
 * Codec <-> codec topology sections.
 *
 * <h4>PCM Configurations</h4>
 * PCM, BE and Codec to Codec link sections define the supported capabilities
 * and configurations for supported playback and capture streams. The
 * definitions and content for PCMs, BE and Codec links are the same with the
 * exception of the section type :-
 *
 * <pre>
 * SectionPCM."name" {
 *	....
 * }
 * SectionBE."name" {
 *	....
 * }
 * SectionCC."name" {
 *	....
 * }
 * </pre>
 *
 * The section types above should be used for PCMs, Back Ends and Codec to Codec
 * links respectively.<br>
 *
 * The data for each section is defined as follows :-
 *
 * <pre>
 * SectionPCM."name" {
 *
 *	index "1"			# Index number
 *
 *	id "0"				# used for binding to the PCM
 *
 *	pcm."playback" {
 *		capabilities "capabilities1"	# capabilities for playback
 *
 *		configs [		# supported configs for playback
 *			"config1"
 *			"config2"
 *		]
 *	}
 *
 *	pcm."capture" {
 *		capabilities "capabilities2"	# capabilities for capture
 *
 *		configs [		# supported configs for capture
 *			"config1"
 *			"config2"
 *			"config3"
 *		]
 *	}
 * }
 * </pre>
 *
 */

/** Maximum number of channels supported in one control */
#define SND_TPLG_MAX_CHAN		8

/** Topology context */
typedef struct snd_tplg snd_tplg_t;

/** Topology object types */
enum snd_tplg_type {
	SND_TPLG_TYPE_TLV = 0,		/*!< TLV Data */
	SND_TPLG_TYPE_MIXER,		/*!< Mixer control*/
	SND_TPLG_TYPE_ENUM,		/*!< Enumerated control */
	SND_TPLG_TYPE_TEXT,		/*!< Text data */
	SND_TPLG_TYPE_DATA,		/*!< Private data */
	SND_TPLG_TYPE_BYTES,		/*!< Byte control */
	SND_TPLG_TYPE_STREAM_CONFIG,	/*!< PCM Stream configuration */
	SND_TPLG_TYPE_STREAM_CAPS,	/*!< PCM Stream capabilities */
	SND_TPLG_TYPE_PCM,		/*!< PCM stream device */
	SND_TPLG_TYPE_DAPM_WIDGET,	/*!< DAPM widget */
	SND_TPLG_TYPE_DAPM_GRAPH,	/*!< DAPM graph elements */
	SND_TPLG_TYPE_BE,		/*!< BE DAI link */
	SND_TPLG_TYPE_CC,		/*!< Hostless codec <-> codec link */
	SND_TPLG_TYPE_MANIFEST,		/*!< Topology manifest */
};

/**
 * \brief Create a new topology parser instance.
 * \return New topology parser instance
 */
snd_tplg_t *snd_tplg_new(void);

/**
 * \brief Free a topology parser instance.
 * \param tplg Topology parser instance
 */
void snd_tplg_free(snd_tplg_t *tplg);

/**
 * \brief Parse and build topology text file into binary file.
 * \param tplg Topology instance.
 * \param infile Topology text input file to be parsed
 * \param outfile Binary topology output file.
 * \return Zero on success, otherwise a negative error code
 */
int snd_tplg_build_file(snd_tplg_t *tplg, const char *infile,
	const char *outfile);

/**
 * \brief Enable verbose reporting of binary file output
 * \param tplg Topology Instance
 * \param verbose Enable verbose output level if non zero
 */
void snd_tplg_verbose(snd_tplg_t *tplg, int verbose);

/** \struct snd_tplg_tlv_template
 * \brief Template type for all TLV objects.
 */
struct snd_tplg_tlv_template {
	int type;	 /*!< TLV type SNDRV_CTL_TLVT_ */
};

/** \struct snd_tplg_tlv_dbscale_template
 * \brief Template type for TLV Scale objects.
 */
struct snd_tplg_tlv_dbscale_template {
	struct snd_tplg_tlv_template hdr;	/*!< TLV type header */
	int min;			/*!< dB minimum value in 0.1dB */
	int step;			/*!< dB step size in 0.1dB */
	int mute;			/*!< is min dB value mute ? */
};

/** \struct snd_tplg_channel_template
 * \brief Template type for single channel mapping.
 */
struct snd_tplg_channel_elem {
	int size;	/*!< size in bytes of this structure */
	int reg;	/*!< channel control register */
	int shift;	/*!< channel shift for control bits */
	int id;		/*!< ID maps to Left, Right, LFE etc */
};

/** \struct snd_tplg_channel_map_template
 * \brief Template type for channel mapping.
 */
struct snd_tplg_channel_map_template {
	int num_channels;	/*!< number of channel mappings */
	struct snd_tplg_channel_elem channel[SND_TPLG_MAX_CHAN];	/*!< mapping */
};

/** \struct snd_tplg_pdata_template
 * \brief Template type for private data objects.
 */
struct snd_tplg_pdata_template {
	unsigned int length;	/*!< data length */
	const void *data;	/*!< data */
};

/** \struct snd_tplg_io_ops_template
 * \brief Template type for object operations mapping.
 */
struct snd_tplg_io_ops_template {
	int get;	/*!< get callback ID */
	int put;	/*!< put callback ID */
	int info;	/*!< info callback ID */
};

/** \struct snd_tplg_ctl_template
 * \brief Template type for control objects.
 */
struct snd_tplg_ctl_template {
	int type;		/*!< Control type */
	const char *name;	/*!< Control name */
	int access;		/*!< Control access */
	struct snd_tplg_io_ops_template ops;	/*!< operations */
	struct snd_tplg_tlv_template *tlv; /*!< non NULL means we have TLV data */
};

/** \struct snd_tplg_mixer_template
 * \brief Template type for mixer control objects.
 */
struct snd_tplg_mixer_template {
	struct snd_tplg_ctl_template hdr;	/*!< control type header */
	struct snd_tplg_channel_map_template *map;	/*!< channel map */
	int min;	/*!< min value for mixer */
	int max;	/*!< max value for mixer */
	int platform_max;	/*!< max value for platform control */
	int invert;	/*!< whether controls bits are inverted */
	struct snd_soc_tplg_private *priv;	/*!< control private data */
};

/** \struct snd_tplg_enum_template
 * \brief Template type for enumerated control objects.
 */
struct snd_tplg_enum_template {
	struct snd_tplg_ctl_template hdr;	/*!< control type header */
	struct snd_tplg_channel_map_template *map;	/*!< channel map */
	int items;	/*!< number of enumerated items in control */
	int mask;	/*!< register mask size */
	const char **texts;	/*!< control text items */
	const int **values;	/*!< control value items */
	struct snd_soc_tplg_private *priv;	/*!< control private data */
};

/** \struct snd_tplg_bytes_template
 * \brief Template type for TLV Scale objects.
 */
struct snd_tplg_bytes_template {
	struct snd_tplg_ctl_template hdr;	/*!< control type header */
	int max;		/*!< max byte control value */
	int mask;		/*!< byte control mask */
	int base;		/*!< base register */
	int num_regs;		/*!< number of registers */
	struct snd_tplg_io_ops_template ext_ops;	/*!< ops mapping */
	struct snd_soc_tplg_private *priv;	/*!< control private data */
};

/** \struct snd_tplg_graph_elem
 * \brief Template type for single DAPM graph element.
 */
struct snd_tplg_graph_elem {
	const char *src;	/*!< source widget name */
	const char *ctl;	/*!< control name or NULL if no control */
	const char *sink;	/*!< sink widget name */
};

/** \struct snd_tplg_graph_template
 * \brief Template type for array of DAPM graph elements.
 */
struct snd_tplg_graph_template {
	int count;		/*!< Number of graph elements */
	struct snd_tplg_graph_elem elem[0];	/*!< graph elements */
};

/** \struct snd_tplg_widget_template
 * \brief Template type for DAPM widget objects.
 */
struct snd_tplg_widget_template {
	int id;			/*!< SND_SOC_DAPM_CTL */
	const char *name;	/*!< widget name */
	const char *sname;	/*!< stream name (certain widgets only) */
	int reg;		/*!< negative reg = no direct dapm */
	int shift;		/*!< bits to shift */
	int mask;		/*!< non-shifted mask */
	int subseq;		/*!< sort within widget type */
	unsigned int invert;		/*!< invert the power bit */
	unsigned int ignore_suspend;	/*!< kept enabled over suspend */
	unsigned short event_flags;	/*!< PM event sequence flags */
	unsigned short event_type;	/*!< PM event sequence type */
	struct snd_soc_tplg_private *priv;	/*!< widget private data */
	int num_ctls;			/*!< Number of controls used by widget */
	struct snd_tplg_ctl_template *ctl[0];	/*!< array of widget controls */
};

/** \struct snd_tplg_stream_template
 * \brief Stream configurations.
 */
struct snd_tplg_stream_template {
	const char *name;	/*!< name of the stream config */
	int format;		/*!< SNDRV_PCM_FMTBIT_* */
	int rate;		/*!< SNDRV_PCM_RATE_* */
	int period_bytes;	/*!< size of period in bytes */
	int buffer_bytes;	/*!< size of buffer in bytes. */
	int channels;		/*!< number of channels */
};

/** \struct snd_tplg_stream_caps_template
 * \brief Stream Capabilities.
 */
struct snd_tplg_stream_caps_template {
	const char *name;	/*!< name of the stream caps */
	uint64_t formats;	/*!< supported formats SNDRV_PCM_FMTBIT_* */
	unsigned int rates;	/*!< supported rates SNDRV_PCM_RATE_* */
	unsigned int rate_min;	/*!< min rate */
	unsigned int rate_max;	/*!< max rate */
	unsigned int channels_min;	/*!< min channels */
	unsigned int channels_max;	/*!< max channels */
	unsigned int periods_min;	/*!< min number of periods */
	unsigned int periods_max;	/*!< max number of periods */
	unsigned int period_size_min;	/*!< min period size bytes */
	unsigned int period_size_max;	/*!< max period size bytes */
	unsigned int buffer_size_min;	/*!< min buffer size bytes */
	unsigned int buffer_size_max;	/*!< max buffer size bytes */
};

/** \struct snd_tplg_pcm_template
 * \brief Template type for PCM (FE DAI & DAI links).
 */
struct snd_tplg_pcm_template {
	const char *pcm_name;	/*!< PCM stream name */
	const char *dai_name;	/*!< DAI name */
	unsigned int pcm_id;	/*!< unique ID - used to match */
	unsigned int dai_id;	/*!< unique ID - used to match */
	unsigned int playback;	/*!< supports playback mode */
	unsigned int capture;	/*!< supports capture mode */
	unsigned int compress;	/*!< 1 = compressed; 0 = PCM */
	struct snd_tplg_stream_caps_template *caps[2]; /*!< playback & capture for DAI */
	int num_streams;	/*!< number of supported configs */
	struct snd_tplg_stream_template stream[0]; /*!< supported configs */
};

/** \struct snd_tplg_link_template
 * \brief Template type for BE and CC DAI Links.
 */
struct snd_tplg_link_template {
	const char *name;	/*!< link name */
	int id;	/*!< unique ID - used to match with existing BE and CC links */
	int num_streams;	/*!< number of configs */
	struct snd_tplg_stream_template stream[0]; /*!< supported configs */
};

/** \struct snd_tplg_obj_template
 * \brief Generic Template Object
 */
typedef struct snd_tplg_obj_template {
	enum snd_tplg_type type;	/*!< template object type */
	int index;		/*!< group index for object */
	int version;		/*!< optional vendor specific version details */
	int vendor_type;	/*!< optional vendor specific type info */
	union {
		struct snd_tplg_widget_template *widget;	/*!< DAPM widget */
		struct snd_tplg_mixer_template *mixer;		/*!< Mixer control */
		struct snd_tplg_bytes_template *bytes_ctl;	/*!< Bytes control */
		struct snd_tplg_enum_template *enum_ctl;	/*!< Enum control */
		struct snd_tplg_graph_template *graph;		/*!< Graph elements */
		struct snd_tplg_pcm_template *pcm;		/*!< PCM elements */
		struct snd_tplg_link_template *link;		/*!< BE and CC Links */
	};
} snd_tplg_obj_template_t;

/**
 * \brief Register topology template object.
 * \param tplg Topology instance.
 * \param t Template object.
 * \return Zero on success, otherwise a negative error code
 */
int snd_tplg_add_object(snd_tplg_t *tplg, snd_tplg_obj_template_t *t);

/**
 * \brief Build all registered topology data into binary file.
 * \param tplg Topology instance.
 * \param outfile Binary topology output file.
 * \return Zero on success, otherwise a negative error code
 */
int snd_tplg_build(snd_tplg_t *tplg, const char *outfile);

/**
 * \brief Attach private data to topology manifest.
 * \param tplg Topology instance.
 * \param data Private data.
 * \param len Length of data in bytes.
 * \return Zero on success, otherwise a negative error code
 */
int snd_tplg_set_manifest_data(snd_tplg_t *tplg, const void *data, int len);

/**
 * \brief Set an optional vendor specific version number.
 * \param tplg Topology instance.
 * \param version Vendor specific version number.
 * \return Zero on success, otherwise a negative error code
 */
int snd_tplg_set_version(snd_tplg_t *tplg, unsigned int version);

/* \} */

#ifdef __cplusplus
}
#endif

#endif /* __ALSA_TOPOLOGY_H */
