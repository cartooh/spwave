/*
 *	swInfoDialog.c
 */

#include <stdio.h>
#include <stdlib.h>

#include <sp/spBaseLib.h>
#include <sp/spAudioLib.h>
#include <sp/spComponentLib.h>

#include "swWindow.h"
#include "swDraw.h"
#include "swEdit.h"
#include "swDialog.h"
#include "swInfoDialog.h"

#ifdef SW_SUPPORT_INFO_DIALOG

static char *sw_genres[] =
{
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop",
    "Jazz", "Metal", "New Age", "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae", "Rock",
    "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks", "Soundtrack",
    "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk", "Fusion", "Trance",
    "Classical", "Instrumental", "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise",
    "Alt", "Bass", "Soul", "Punk", "Space", "Meditative", "Instrumental Pop",
    "Instrumental Rock", "Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
    "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult",
    "Gangsta Rap", "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American",
    "Cabaret", "New Wave", "Psychedelic", "Rave", "Showtunes", "Trailer", "Lo-Fi",
    "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical", "Rock & Roll",
    "Hard Rock", "Folk", "Folk/Rock", "National Folk", "Swing", "Fast-Fusion", "Bebob",
    "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde", "Gothic Rock",
    "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",
    "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson", "Opera",
    "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus", "Porn Groove",
    "Satire", "Slow Jam", "Club", "Tango", "Samba", "Folklore", "Ballad", "Power Ballad",
    "Rhythmic Soul", "Freestyle", "Duet", "Punk Rock", "Drum Solo", "A Cappella",
    "Euro-House", "Dance Hall", "Goa", "Drum & Bass", "Club-House", "Hardcore", "Terror",
    "Indie", "BritPop", "Negerpunk", "Polsk Punk", "Beat", "Christian Gangsta Rap",
    "Heavy Metal", "Black Metal", "Crossover", "Contemporary Christian",
    "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop", "Synthpop",
    "Unknown Genre", NULL,
};

static char *sw_tracks[] =
{
    "", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
    "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
    "21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
    "31", "32", "33", "34", "35", "36", "37", "38", "39", "40",
    "41", "42", "43", "44", "45", "46", "47", "48", "49", "50",
    NULL,
};

static char *sw_discs[] =
{
    "", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
    NULL,
};

static char *sw_id3_versions[] =
{
    "Not ID3v2",
    "ID3v2.2",
    "ID3v2.3",
    "ID3v2.4",
    NULL,
};

static void setInfoString(unsigned long mask1, unsigned long mask2,
			  spComponent text, char *string, spBool update_text)
{
    spDebug(10, "setInfoString", "mask1 = %08lx, mask2 = %08lx\n", mask1, mask2);
    
    if (mask1 & mask2) {
	spDebug(10, "setInfoString", "string = %s\n", string);
    
	spSetSensitive(text, SP_TRUE);
	if (update_text) {
	    spSetTextString(text, string);
	}
    } else {
	if (update_text) {
	    spSetTextString(text, "");
	}
	spSetSensitive(text, SP_FALSE);
    }

    return;
}

static void setInfoLong32(unsigned long mask1, unsigned long mask2,
			  spComponent text, spLong32 number, spBool update_text)
{
    char buf[SP_MAX_LINE];
    
    if (number > 0) {
	sprintf(buf, "%ld", (long)number);
    } else {
	strcpy(buf, "");
    }
    setInfoString(mask1, mask2, text, buf, update_text);
    
    return;
}

static int getID3v2Index(unsigned long support_mask, unsigned long current_mask, spBool *id3v2_supported)
{
    int index = 0;

    spDebug(10, "getID3v2Index", "support_mask = %lx, current_mask = %lx\n", support_mask, current_mask);
	
    if ((support_mask & SP_SONG_ID3V2_2_MASK) || (support_mask & SP_SONG_ID3V2_3_MASK) || (support_mask & SP_SONG_ID3V2_4_MASK)) {
	if (id3v2_supported != NULL) {
	    *id3v2_supported = SP_TRUE;
	}
	
	if (current_mask & SP_SONG_ID3V2_4_MASK) {
	    spDebug(10, "getID3v2Index", "SP_SONG_ID3V2_4_MASK\n");
	    index = 3;
	} else if (current_mask & SP_SONG_ID3V2_3_MASK) {
	    spDebug(10, "getID3v2Index", "SP_SONG_ID3V2_3_MASK\n");
	    index = 2;
	} else if (current_mask & SP_SONG_ID3V2_2_MASK) {
	    spDebug(10, "getID3v2Index", "SP_SONG_ID3V2_2_MASK\n");
	    index = 1;
	}
    } else {
        spDebug(10, "getID3v2Index", "ID3v2 not supported\n");
	if (id3v2_supported != NULL) {
	    *id3v2_supported = SP_FALSE;
	}
    }

    return index;
}

static void setID3v2(spComponent combo, unsigned long support_mask, unsigned long current_mask, spBool update_text)
{
    int index;
    spBool id3v2_supported = SP_FALSE;

    index = getID3v2Index(support_mask, current_mask, &id3v2_supported);
    spDebug(10, "setID3v2", "index = %d, id3v2_supported = %d\n", index, id3v2_supported);

    if (update_text) {
	spSelectListIndex(combo, index);
    }
    
    if (id3v2_supported) {
	spSetSensitive(combo, SP_TRUE);
    } else {
	spSetSensitive(combo, SP_FALSE);
    }
    
    spDebug(80, "setID3v2", "done\n");
    
    return;
}

static int updateCurrentID3Mask(int id3_version_index, unsigned long support_mask, unsigned long *current_mask)
{
    int index = 0;
    
    if ((support_mask & SP_SONG_ID3V2_2_MASK) || (support_mask & SP_SONG_ID3V2_3_MASK) || (support_mask & SP_SONG_ID3V2_4_MASK)) {
	if (id3_version_index == 3 && (support_mask & SP_SONG_ID3V2_4_MASK)) {
	    spDebug(10, "updateCurrentID3Mask", "SP_SONG_ID3V2_4_MASK\n");
	    index = 3;
	} else if (id3_version_index >= 2 && (support_mask & SP_SONG_ID3V2_3_MASK)) {
	    spDebug(10, "updateCurrentID3Mask", "SP_SONG_ID3V2_3_MASK\n");
	    index = 2;
	} else if (id3_version_index >= 1 && (support_mask & SP_SONG_ID3V2_2_MASK)) {
	    spDebug(10, "updateCurrentID3Mask", "SP_SONG_ID3V2_2_MASK\n");
	    index = 1;
	}
    }

    if (index == 3) {
	*current_mask &= ~(SP_SONG_ID3V2_2_MASK | SP_SONG_ID3V2_3_MASK);
	*current_mask |= SP_SONG_ID3V2_4_MASK;
    } else if (index == 2) {
	*current_mask &= ~(SP_SONG_ID3V2_2_MASK | SP_SONG_ID3V2_4_MASK);
	*current_mask |= SP_SONG_ID3V2_3_MASK;
    } else if (index == 1) {
	*current_mask &= ~(SP_SONG_ID3V2_3_MASK | SP_SONG_ID3V2_4_MASK);
	*current_mask |= SP_SONG_ID3V2_2_MASK;
    } else {
	*current_mask &= ~(SP_SONG_ID3V2_2_MASK | SP_SONG_ID3V2_3_MASK | SP_SONG_ID3V2_4_MASK);
    }

    return index;
}

void swUpdateInfoDialog(swInfoDialog info_dialog, swWindow window, int id3_version_index, spBool update_text)
{
    unsigned long support_mask;
    
    if (info_dialog == NULL || window->wave == NULL) return;

    if (id3_version_index <= 0) {
	support_mask = info_dialog->support_mask;
    } else {
	support_mask = SP_SONG_ID3V2_ALL_SUPPORT_MASK;
    }
    spDebug(10, "swUpdateInfoDialog", "id3_version_index = %d, support_mask = %lx\n", id3_version_index, support_mask);

    setInfoString(support_mask, SP_SONG_TITLE_MASK,
		  info_dialog->title_text, window->wave->song_info.title, update_text);
    setInfoString(support_mask, SP_SONG_ARTIST_MASK,
		  info_dialog->artist_text, window->wave->song_info.artist, update_text);
    setInfoString(support_mask, SP_SONG_ALBUM_MASK,
		  info_dialog->album_text, window->wave->song_info.album, update_text);
    setInfoString(support_mask, SP_SONG_ALBUM_ARTIST_MASK,
		  info_dialog->album_artist_text, window->wave->song_info.album_artist, update_text);
    setInfoString(support_mask, SP_SONG_COMPOSER_MASK,
		  info_dialog->composer_text, window->wave->song_info.composer, update_text);
    setInfoString(support_mask, SP_SONG_LYRICIST_MASK,
		  info_dialog->lyricist_text, window->wave->song_info.lyricist, update_text);
    setInfoString(support_mask, SP_SONG_GENRE_MASK,
		  info_dialog->genre_text, window->wave->song_info.genre, update_text);
    setID3v2(info_dialog->id3_version_combo, support_mask, window->wave->song_info.info_mask, update_text);
    setInfoString(support_mask, SP_SONG_RELEASE_MASK,
		  info_dialog->release_text, window->wave->song_info.release, update_text);
    setInfoString(support_mask, SP_SONG_COPYRIGHT_MASK,
		  info_dialog->copyright_text, window->wave->song_info.copyright, update_text);
    setInfoString(support_mask, SP_SONG_PRODUCER_MASK,
		  info_dialog->producer_text, window->wave->song_info.producer, update_text);
    setInfoString(support_mask, SP_SONG_ENGINEER_MASK,
		  info_dialog->engineer_text, window->wave->song_info.engineer, update_text);
    setInfoString(support_mask, SP_SONG_SOURCE_MASK,
		  info_dialog->source_text, window->wave->song_info.source, update_text);
    setInfoString(support_mask, SP_SONG_ISRC_MASK,
		  info_dialog->isrc_text, window->wave->song_info.isrc, update_text);
    setInfoString(support_mask, SP_SONG_SOFTWARE_MASK,
		  info_dialog->software_text, window->wave->song_info.software, update_text);
    setInfoString(support_mask, SP_SONG_SUBJECT_MASK,
		  info_dialog->subject_text, window->wave->song_info.subject, update_text);
    setInfoString(support_mask, SP_SONG_COMMENT_MASK,
		  info_dialog->comment_text, window->wave->song_info.comment, update_text);
    setInfoLong32(support_mask, SP_SONG_TRACK_MASK,
		  info_dialog->track_text, window->wave->song_info.track, update_text);
    setInfoLong32(support_mask, SP_SONG_TRACK_TOTAL_MASK,
		  info_dialog->track_total_text, window->wave->song_info.track_total, update_text);
    setInfoLong32(support_mask, SP_SONG_DISC_MASK,
		  info_dialog->disc_text, window->wave->song_info.disc, update_text);
    setInfoLong32(support_mask, SP_SONG_DISC_TOTAL_MASK,
		  info_dialog->disc_total_text, window->wave->song_info.disc_total, update_text);
    setInfoLong32(support_mask, SP_SONG_TEMPO_MASK,
		  info_dialog->tempo_text, window->wave->song_info.tempo, update_text);

    spDebug(80, "swUpdateInfoDialog", "done\n");
    
    return;
}

static void swSelectID3VersionCB(spComponent component, swInfoDialog dialog)
{
    int index;

    if (dialog->config->toplevel->current_window == NULL || dialog->config->toplevel->current_window->wave == NULL) {
	return;
    }
    
    if ((index = spGetSelectedListIndex(component)) >= 0) {
	spDebug(50, "swSelectID3VersionCB", "index = %d\n", index);
	index = updateCurrentID3Mask(index, dialog->support_mask, &dialog->current_mask);
	swUpdateInfoDialog(dialog, dialog->config->toplevel->current_window, index, SP_FALSE);
    }
    
    return;
}

static swInfoDialog createInfoDialog(swConfig config)
{
    swInfoDialog dialog;
    int field_height = 0;
    int field_offset = 100;
    int field_size = /*260*/-1;
    int field_spacing = 20;
    int genre_field_size = 110;
    int id3_version_field_offset = 50;
    int id3_version_field_size = /*50*/-1;
    int release_field_size = 110;
    int tempo_field_offset = 50;
    int tempo_field_size = /*50*/-1;
    int track_field_size = 40;
    int track_total_field_offset = 8;
    int disc_field_offset = 50;
    int disc_field_size = 40;
    int disc_total_field_offset = 8;
    
    dialog = xalloc(1, struct _swInfoDialog);
    memset(dialog, 0, sizeof(struct _swInfoDialog));
    dialog->config = config;
    
    /* create dialog */
    dialog->window = spCreateDialogBox("infoDialog",
				       SppTitle, SW_INFO_DIALOG_TITLE,
				       SppCallbackFunc, swPopdownInfoDialogCB,
				       SppCallbackData, dialog,
				       SppDialogBoxButtonType, SP_DB_OK_CANCEL,
				       SppCloseStyle, SP_UNMAP_CLOSE,
				       SppHelpButtonVisible, SP_TRUE,
				       SppHelpPath, "dialog/file_metadata.html",
				       NULL);
    
    /* create text field */
    dialog->title_text = spCreateParamField(dialog->window, "infoTitleText", field_height,
					    SppTitle, SW_INFO_DIALOG_TITLE_LABEL,
					    SppFieldType, SP_FIELD_TYPE_TEXT,
					    SppEditable, SP_TRUE,
					    SppFieldOffset, field_offset,
					    SppFieldSize, field_size,
					    SppHelpPath, "dialog/file_metadata.html#file_metadata_title",
					    NULL);
    dialog->artist_text = spCreateParamField(dialog->window, "infoArtistText", field_height,
					     SppTitle, SW_INFO_DIALOG_ARTIST_LABEL,
					     SppFieldType, SP_FIELD_TYPE_TEXT,
					     SppEditable, SP_TRUE,
					     SppFieldOffset, field_offset,
					     SppFieldSize, field_size,
					     SppHelpPath, "dialog/file_metadata.html#file_metadata_artist",
					     NULL);
    dialog->album_text = spCreateParamField(dialog->window, "infoAlbumText", field_height,
					    SppTitle, SW_INFO_DIALOG_ALBUM_LABEL,
					    SppFieldType, SP_FIELD_TYPE_TEXT,
					    SppEditable, SP_TRUE,
					    SppFieldOffset, field_offset,
					    SppFieldSize, field_size,
					    SppHelpPath, "dialog/file_metadata.html#file_metadata_album",
					    NULL);
    dialog->album_artist_text = spCreateParamField(dialog->window, "infoAlbumArtistText", field_height,
						   SppTitle, SW_INFO_DIALOG_ALBUM_ARTIST_LABEL,
						   SppFieldType, SP_FIELD_TYPE_TEXT,
						   SppEditable, SP_TRUE,
						   SppFieldOffset, field_offset,
						   SppFieldSize, field_size,
						   SppHelpPath, "dialog/file_metadata.html#file_metadata_album_artist",
						   NULL);
    
    dialog->container = spCreateBox(dialog->window, "infoContainer", 0,
				    SppOrientation, SP_HORIZONTAL,
				    SppUseTextHeight, SP_TRUE,
				    NULL);
    dialog->genre_text = spCreateParamField(dialog->container, "infoGenreText",
					    field_height + genre_field_size + field_spacing,
					    SppTitle, SW_INFO_DIALOG_GENRE_LABEL,
					    SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					    SppEditable, SP_TRUE,
					    SppFieldStrings, sw_genres,
					    SppFieldOffset, field_offset,
					    SppFieldSize, genre_field_size,
					    SppHelpPath, "dialog/file_metadata.html#file_metadata_genre",
					    NULL);
    if (id3_version_field_size > 0) {
	id3_version_field_size = MAX(id3_version_field_size, field_size - (genre_field_size + field_spacing + id3_version_field_offset));
    }
    dialog->id3_version_combo = spCreateParamField(dialog->container, "infoID3VersionCombo",
                                                   id3_version_field_size > 0 ? field_spacing + id3_version_field_offset + id3_version_field_size : -1,
                                                   SppTitle, SW_INFO_DIALOG_ID3_VERSION_LABEL,
                                                   SppCallbackFunc, swSelectID3VersionCB,
                                                   SppCallbackData, dialog,
                                                   SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
                                                   SppSelectOnly, SP_TRUE,
                                                   SppFieldStrings, sw_id3_versions,
                                                   SppFieldOffset, id3_version_field_offset,
                                                   SppFieldSize, id3_version_field_size,
                                                   SppHelpPath, "dialog/file_metadata.html#file_metadata_id3_version",
                                                   NULL);
    
    dialog->composer_text = spCreateParamField(dialog->window, "infoComposerText", field_height,
					       SppTitle, SW_INFO_DIALOG_COMPOSER_LABEL,
					       SppFieldType, SP_FIELD_TYPE_TEXT,
					       SppEditable, SP_TRUE,
					       SppFieldOffset, field_offset,
					       SppFieldSize, field_size,
					       SppHelpPath, "dialog/file_metadata.html#file_metadata_composer",
					       NULL);
    dialog->lyricist_text = spCreateParamField(dialog->window, "infoLyricistText", field_height,
					       SppTitle, SW_INFO_DIALOG_LYRICIST_LABEL,
					       SppFieldType, SP_FIELD_TYPE_TEXT,
					       SppEditable, SP_TRUE,
					       SppFieldOffset, field_offset,
					       SppFieldSize, field_size,
					       SppHelpPath, "dialog/file_metadata.html#file_metadata_lyricist",
					       NULL);
    dialog->producer_text = spCreateParamField(dialog->window, "infoProducerText", field_height,
					       SppTitle, SW_INFO_DIALOG_PRODUCER_LABEL,
					       SppFieldType, SP_FIELD_TYPE_TEXT,
					       SppEditable, SP_TRUE,
					       SppFieldOffset, field_offset,
					       SppFieldSize, field_size,
					       SppHelpPath, "dialog/file_metadata.html#file_metadata_producer",
					       NULL);
    dialog->engineer_text = spCreateParamField(dialog->window, "infoEngineerText", field_height,
					       SppTitle, SW_INFO_DIALOG_ENGINEER_LABEL,
					       SppFieldType, SP_FIELD_TYPE_TEXT,
					       SppEditable, SP_TRUE,
					       SppFieldOffset, field_offset,
					       SppFieldSize, field_size,
					       SppHelpPath, "dialog/file_metadata.html#file_metadata_engineer",
					       NULL);

    dialog->container2 = spCreateBox(dialog->window, "infoContainer2", 0,
				    SppOrientation, SP_HORIZONTAL,
				    SppUseTextHeight, SP_TRUE,
				    NULL);
    dialog->release_text = spCreateParamField(dialog->container2, "infoReleaseText",
					      field_offset + release_field_size + field_spacing,
					      SppTitle, SW_INFO_DIALOG_RELEASE_LABEL,
					      SppFieldType, SP_FIELD_TYPE_TEXT,
					      SppEditable, SP_TRUE,
					      SppFieldOffset, field_offset,
					      SppFieldSize, release_field_size,
					      SppHelpPath, "dialog/file_metadata.html#file_metadata_release",
					      NULL);
    if (tempo_field_size > 0) {
	tempo_field_size = MAX(tempo_field_size, field_size - (release_field_size + field_spacing + tempo_field_offset));
    }
    dialog->tempo_text = spCreateParamField(dialog->container2, "infoTempoText",
					    tempo_field_size > 0 ? field_spacing + tempo_field_offset + tempo_field_size : -1,
					    SppTitle, SW_INFO_DIALOG_TEMPO_LABEL,
					    SppFieldType, SP_FIELD_TYPE_TEXT,
					    SppEditable, SP_TRUE,
					    SppFieldOffset, tempo_field_offset,
					    SppFieldSize, tempo_field_size,
					    SppHelpPath, "dialog/file_metadata.html#file_metadata_tempo",
					    NULL);

    dialog->container3 = spCreateBox(dialog->window, "infoContainer3", 0,
				     SppOrientation, SP_HORIZONTAL,
				     SppUseTextHeight, SP_TRUE,
				     NULL);
    dialog->track_text = spCreateParamField(dialog->container3, "infoTrackText",
					    field_offset + track_field_size,
					    SppTitle, SW_INFO_DIALOG_TRACK_LABEL,
					    SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					    SppEditable, SP_TRUE,
					    SppFieldStrings, sw_tracks,
					    SppFieldOffset, field_offset,
					    SppFieldSize, track_field_size,
					    SppHelpPath, "dialog/file_metadata.html#file_metadata_track",
					    NULL);
    dialog->track_total_text = spCreateParamField(dialog->container3, "infoTrackTotalText",
						  track_total_field_offset + track_field_size,
						  SppTitle, "/",
						  SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						  SppEditable, SP_TRUE,
						  SppFieldStrings, sw_tracks,
						  SppFieldOffset, track_total_field_offset,
						  SppFieldSize, track_field_size,
						  SppHelpPath, "dialog/file_metadata.html#file_metadata_track_total",
						  NULL);

    dialog->disc_text = spCreateParamField(dialog->container3, "infoDiscText",
					   field_spacing + disc_field_offset + disc_field_size,
					   SppTitle, SW_INFO_DIALOG_DISC_LABEL,
					   SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
					   SppEditable, SP_TRUE,
					   SppFieldStrings, sw_discs,
					   SppFieldOffset, disc_field_offset,
					   SppFieldSize, disc_field_size,
					   SppHelpPath, "dialog/file_metadata.html#file_metadata_disc",
					   NULL);
    dialog->disc_total_text = spCreateParamField(dialog->container3, "infoDiscTotalText",
						 disc_total_field_offset + disc_field_size,
						 SppTitle, "/",
						 SppFieldType, SP_FIELD_TYPE_COMBO_BOX,
						 SppEditable, SP_TRUE,
						 SppFieldStrings, sw_discs,
						 SppFieldOffset, disc_total_field_offset,
						 SppFieldSize, disc_field_size,
						 SppHelpPath, "dialog/file_metadata.html#file_metadata_disc_total",
						 NULL);
    
    dialog->copyright_text = spCreateParamField(dialog->window, "infoCopyrightText", field_height,
						SppTitle, SW_INFO_DIALOG_COPYRIGHT_LABEL,
						SppFieldType, SP_FIELD_TYPE_TEXT,
						SppEditable, SP_TRUE,
						SppFieldOffset, field_offset,
						SppFieldSize, field_size,
						SppHelpPath, "dialog/file_metadata.html#file_metadata_copyright",
						NULL);
    dialog->source_text = spCreateParamField(dialog->window, "infoSourceText", field_height,
					     SppTitle, SW_INFO_DIALOG_SOURCE_LABEL,
					     SppFieldType, SP_FIELD_TYPE_TEXT,
					     SppEditable, SP_TRUE,
					     SppFieldOffset, field_offset,
					     SppFieldSize, field_size,
					     SppHelpPath, "dialog/file_metadata.html#file_metadata_source",
					     NULL);
    dialog->isrc_text = spCreateParamField(dialog->window, "infoIsrcText", field_height,
					     SppTitle, SW_INFO_DIALOG_ISRC_LABEL,
					     SppFieldType, SP_FIELD_TYPE_TEXT,
					     SppEditable, SP_TRUE,
					     SppFieldOffset, field_offset,
					     SppFieldSize, field_size,
					     SppHelpPath, "dialog/file_metadata.html#file_metadata_isrc",
					     NULL);
    dialog->software_text = spCreateParamField(dialog->window, "infoSoftwareText", field_height,
					       SppTitle, SW_INFO_DIALOG_SOFTWARE_LABEL,
					       SppFieldType, SP_FIELD_TYPE_TEXT,
					       SppEditable, SP_TRUE,
					       SppFieldOffset, field_offset,
					       SppFieldSize, field_size,
					       SppHelpPath, "dialog/file_metadata.html#file_metadata_software",
					       NULL);
    dialog->subject_text = spCreateParamField(dialog->window, "infoSubjectText", field_height,
					      SppTitle, SW_INFO_DIALOG_SUBJECT_LABEL,
					      SppFieldType, SP_FIELD_TYPE_TEXT,
					      SppEditable, SP_TRUE,
					      SppFieldOffset, field_offset,
					      SppFieldSize, field_size,
					      SppHelpPath, "dialog/file_metadata.html#file_metadata_subject",
					      NULL);
    dialog->comment_text = spCreateParamField(dialog->window, "infoCommentText", field_height,
					      SppTitle, SW_INFO_DIALOG_COMMENT_LABEL,
					      SppFieldType, SP_FIELD_TYPE_TEXT,
					      SppEditable, SP_TRUE,
					      SppFieldOffset, field_offset,
					      SppFieldSize, field_size,
					      SppHelpPath, "dialog/file_metadata.html#file_metadata_comment",
					      NULL);
    
    return dialog;
}

swInfoDialog swCreateInfoDialog(swConfig config)
{
    static swInfoDialog info_dialog = NULL;
    
    if (info_dialog == NULL) {
	info_dialog = createInfoDialog(config);
    }
    return info_dialog;
}

void swPopupInfoDialogCB(spComponent component, swWindow window)
{
    int id3_version_index;
    swInfoDialog info_dialog;
    
    window->config->toplevel->current_window = window;
    
    info_dialog = swCreateInfoDialog(window->config);

    info_dialog->support_mask = swGetSongInfoMask(window->wave);
    if (window->wave != NULL) {
	info_dialog->current_mask = window->wave->song_info.info_mask;
    } else {
	info_dialog->current_mask = 0L;
    }
    spDebug(80, "swPopupInfoDialogCB", "info_dialog->support_mask = %lx, info_dialog->current_mask = %ld\n",
            info_dialog->support_mask, info_dialog->current_mask);
    
    id3_version_index = getID3v2Index(info_dialog->support_mask, info_dialog->current_mask, NULL);
    spDebug(80, "swPopupInfoDialogCB", "id3_version_index = %d\n", id3_version_index);
    swUpdateInfoDialog(info_dialog, window, id3_version_index, SP_TRUE);

    /* popup dialog */
    spPopupWindow(info_dialog->window);

    spDebug(80, "swPopupInfoDialogCB", "done\n");
    
    return;
}

static spBool getInfoString(unsigned long *mask1, unsigned long mask2,
			    spComponent text, char *string, int size)
{
    const char *p;
    
    if (spIsSensitive(text) == SP_TRUE) {
	if ((p = spGetTextString(text)) != NULL && !strnone(p)) {
	    *mask1 |= mask2;
	    spStrCopy(string, size, p);
	    spDebug(10, "getInfoString", "string = %s\n", string);
	    return SP_TRUE;
	}
    }

    return SP_FALSE;
}

static spBool getInfoLong32(unsigned long *mask1, unsigned long mask2,
			    spComponent text, spLong32 *number)
{
    long num;
    char buf[SP_SONG_INFO_SIZE];
    
    if (getInfoString(mask1, mask2, text, buf, sizeof(buf)) == SP_TRUE) {
	num = 0;
	if (sscanf(buf, "%ld", &num) <= 0) {
	    *mask1 &= ~mask2;
	} else {
	    *number = num;
	    return SP_TRUE;
	}
    }

    return SP_FALSE;
}

static spBool getID3v2Mask(spComponent combo, unsigned long support_mask, unsigned long *current_mask)
{
    int index;

    if ((index = spGetSelectedListIndex(combo)) >= 1
	&& ((support_mask & SP_SONG_ID3V2_2_MASK) || (support_mask & SP_SONG_ID3V2_3_MASK) || (support_mask & SP_SONG_ID3V2_4_MASK))) {
	if (index == 3 && (support_mask & SP_SONG_ID3V2_4_MASK)) {
	    *current_mask &= ~(SP_SONG_ID3V2_2_MASK | SP_SONG_ID3V2_3_MASK);
	    *current_mask |= SP_SONG_ID3V2_4_MASK;
	} else if (index == 2 && (support_mask & SP_SONG_ID3V2_3_MASK)) {
	    *current_mask &= ~(SP_SONG_ID3V2_2_MASK | SP_SONG_ID3V2_4_MASK);
	    *current_mask |= SP_SONG_ID3V2_3_MASK;
	} else {
	    *current_mask &= ~(SP_SONG_ID3V2_3_MASK | SP_SONG_ID3V2_4_MASK);
	    *current_mask |= SP_SONG_ID3V2_2_MASK;
	}
    } else {
	*current_mask &= ~(SP_SONG_ID3V2_2_MASK | SP_SONG_ID3V2_3_MASK | SP_SONG_ID3V2_4_MASK);
    }

    return SP_TRUE;
}

void swPopdownInfoDialogCB(spComponent component, swInfoDialog dialog)
{
    spCallbackReason reason = SP_CR_NONE;
    swWindow window;
    spSongInfoV2 song_info;

    window = dialog->config->toplevel->current_window;

    if (dialog == NULL || spIsCreated(dialog->window) == SP_FALSE
	|| window == NULL) return;

    reason = spGetCallbackReason(component);

    if (reason == SP_CR_OK || reason == SP_CR_CANCEL) {
	/* popdown dialog */
	spPopdownWindow(dialog->window);
    }
    
    if (reason == SP_CR_OK || reason == SP_CR_APPLY) {
	spInitSongInfoV2(&song_info);
	
	getInfoString(&song_info.info_mask, SP_SONG_TITLE_MASK, 
		      dialog->title_text, song_info.title, SP_SONG_INFO_SIZE);
	getInfoString(&song_info.info_mask, SP_SONG_ARTIST_MASK,
		      dialog->artist_text, song_info.artist, SP_SONG_INFO_SIZE);
	getInfoString(&song_info.info_mask, SP_SONG_ALBUM_MASK,
		      dialog->album_text, song_info.album, SP_SONG_INFO_SIZE);
	getInfoString(&song_info.info_mask, SP_SONG_ALBUM_ARTIST_MASK,
		      dialog->album_artist_text, song_info.album_artist, SP_SONG_INFO_ALBUM_ARTIST_SIZE);
	getInfoString(&song_info.info_mask, SP_SONG_COMPOSER_MASK,
		      dialog->composer_text, song_info.composer, SP_SONG_INFO_COMPOSER_SIZE);
	getInfoString(&song_info.info_mask, SP_SONG_LYRICIST_MASK,
		      dialog->lyricist_text, song_info.lyricist, SP_SONG_INFO_LYRICIST_SIZE);
	getInfoString(&song_info.info_mask, SP_SONG_GENRE_MASK,
		      dialog->genre_text, song_info.genre, SP_SONG_INFO_GENRE_SIZE);
	getID3v2Mask(dialog->id3_version_combo, dialog->support_mask, &song_info.info_mask);
	getInfoString(&song_info.info_mask, SP_SONG_RELEASE_MASK,
		      dialog->release_text, song_info.release, SP_SONG_INFO_RELEASE_SIZE);
	getInfoString(&song_info.info_mask, SP_SONG_COPYRIGHT_MASK,
		      dialog->copyright_text, song_info.copyright, SP_SONG_INFO_SIZE);
	getInfoString(&song_info.info_mask, SP_SONG_PRODUCER_MASK,
		      dialog->producer_text, song_info.producer, SP_SONG_INFO_PRODUCER_SIZE);
	getInfoString(&song_info.info_mask, SP_SONG_ENGINEER_MASK,
		      dialog->engineer_text, song_info.engineer, SP_SONG_INFO_SIZE);
	getInfoString(&song_info.info_mask, SP_SONG_SOURCE_MASK,
		      dialog->source_text, song_info.source, SP_SONG_INFO_SOURCE_SIZE);
	getInfoString(&song_info.info_mask, SP_SONG_ISRC_MASK,
		      dialog->isrc_text, song_info.isrc, SP_SONG_INFO_ISRC_SIZE);
	getInfoString(&song_info.info_mask, SP_SONG_SOFTWARE_MASK,
		      dialog->software_text, song_info.software, SP_SONG_INFO_SOFTWARE_SIZE);
	getInfoString(&song_info.info_mask, SP_SONG_SUBJECT_MASK,
		      dialog->subject_text, song_info.subject, SP_SONG_INFO_SUBJECT_SIZE);
	getInfoString(&song_info.info_mask, SP_SONG_COMMENT_MASK,
		      dialog->comment_text, song_info.comment, SP_SONG_INFO_SIZE);

	getInfoLong32(&song_info.info_mask, SP_SONG_TRACK_MASK,
		      dialog->track_text, &song_info.track);
	getInfoLong32(&song_info.info_mask, SP_SONG_TRACK_TOTAL_MASK,
		      dialog->track_total_text, &song_info.track_total);
	getInfoLong32(&song_info.info_mask, SP_SONG_DISC_MASK,
		      dialog->disc_text, &song_info.disc);
	getInfoLong32(&song_info.info_mask, SP_SONG_DISC_TOTAL_MASK,
		      dialog->disc_total_text, &song_info.disc_total);
	getInfoLong32(&song_info.info_mask, SP_SONG_TEMPO_MASK,
		      dialog->tempo_text, &song_info.tempo);

	if (spEqSongInfoV2(&window->wave->song_info, &song_info) == SP_FALSE) {
	    swEditWindow(window, SW_EDIT_CROP, 0, window->wave->total_length, 0.0);
	    spCopySongInfoV2(&window->wave->song_info, &song_info);
	}
    }
    
    return;
}
#endif
