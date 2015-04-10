///////////////////////////////////////////////////////////////////////
// File:        ltrresultiterator.cpp
// Description: Iterator for tesseract results in strict left-to-right
//              order that avoids using tesseract internal data structures.
// Author:      Ray Smith
// Created:     Fri Feb 26 14:32:09 PST 2010
//
// (C) Copyright 2010, Google Inc.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
///////////////////////////////////////////////////////////////////////

#include "ltrresultiterator.h"

#include "allheaders.h"
#include "pageres.h"
#include "strngs.h"
#include "tesseractclass.h"

namespace tesseract {

    LTRResultIterator::LTRResultIterator(PAGE_RES* page_res, Tesseract* tesseract,
            int scale, int scaled_yres,
            int rect_left, int rect_top,
            int rect_width, int rect_height)
    : PageIterator(page_res, tesseract, scale, scaled_yres,
    rect_left, rect_top, rect_width, rect_height),
    line_separator_("\n"),
    paragraph_separator_("\n") {
    }

    LTRResultIterator::~LTRResultIterator() {
    }

    // Returns the null terminated UTF-8 encoded text string for the current
    // object at the given level. Use delete [] to free after use.

    char* LTRResultIterator::GetUTF8Text_APP(PageIteratorLevel level, const char* id_user, const char* id_img, int* word_count) const {
        if (it_->word() == NULL) return NULL; // Already at the end!
        STRING text;
        PAGE_RES_IT res_it(*it_);
        WERD_CHOICE* best_choice = res_it.word()->best_choice;
        ASSERT_HOST(best_choice != NULL);
        if (level == RIL_SYMBOL) {
            text = res_it.word()->BestUTF8(blob_index_, false);
        } else if (level == RIL_WORD) {
            text = best_choice->unichar_string();
        } else {
            bool eol = false; // end of line?
            bool eop = false; // end of paragraph?
            do { // for each paragraph in a block
                do { // for each text line in a paragraph
                    do { // for each word in a text line
                        
                        STRING word_info_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
                        
                        best_choice = res_it.word()->best_choice;
                        
                        // IMPLEMENTACAO APP ==========================================
                        TBOX box = res_it.word()->word->bounding_box();
                        int left, top, right, bottom;
                        const int pix_height = pixGetHeight(res_it.word()->tesseract->pix_binary());
                        const int pix_width = pixGetWidth(res_it.word()->tesseract->pix_binary());
                        left = ClipToRange(static_cast<int>(box.left()), 0, pix_width);
                        top = ClipToRange(pix_height - box.top(), 0, pix_height);
                        right = ClipToRange(static_cast<int>(box.right()), left, pix_width);
                        bottom = ClipToRange(pix_height - box.bottom(), top, pix_height);
                        Box* bbox = boxCreate(left, top, right - left, bottom - top);
                        Pix* pix = pixClipRectangle(res_it.word()->tesseract->pix_binary(), bbox, NULL);
                        char buf[256];
                        char buf_i[256];
                        char word_certainty[256];
                        
                        //tprintf("%s : %d\n",best_choice->unichar_string().string(), pix->data);
                        //tprintf("%d\n", *word_count);
                        snprintf(buf_i, sizeof(buf_i), "%07d", *word_count);
                        snprintf(buf, sizeof(buf), "%s%s/%s.jpg", id_user, id_img, buf_i);
                        pixWrite(buf, pix, IFF_JFIF_JPEG);
                        pixDestroy(&pix);
                        boxDestroy(&bbox);
                        //tprintf("%s\n", buf);
                        // ===========================================================
                        
                        snprintf(word_certainty, sizeof(word_certainty), "%f", best_choice->certainty());
                        
                        word_info_xml += "<word_info>\n <text><![CDATA[";
                        word_info_xml += best_choice->unichar_string().string();
                        word_info_xml += "]]></text>\n<word_certainty>";
                        word_info_xml += word_certainty;
                        word_info_xml += "</word_certainty>\n<img_id>";
                        word_info_xml += buf_i;
                        word_info_xml += "</img_id>\n</word_info>";
                        
                        *word_count = *word_count + 1;
                        ASSERT_HOST(best_choice != NULL);
                        //text += best_choice->unichar_string();
                        text += buf_i;
                        text += " ";
                        res_it.forward();
                        eol = res_it.row() != res_it.prev_row();
                        char outfile[256] = "";
                        snprintf(outfile, sizeof(outfile), "%s%s/%s.xml", id_user, id_img, buf_i);
                        FILE* fout = fopen(outfile, "wb");
                        if (fout == NULL) {
                            fprintf(stderr, "Cannot create output file %s\n", outfile);
                            exit(1);
                        }
                        fwrite(word_info_xml.string(), 1, word_info_xml.length(), fout);
                        fclose(fout);
                        
                    } while (!eol);
                    text.truncate_at(text.length() - 1);
                    text += line_separator_;
                    eop = res_it.block() != res_it.prev_block() ||
                            res_it.row()->row->para() != res_it.prev_row()->row->para();
                } while (level != RIL_TEXTLINE && !eop);
                if (eop) text += paragraph_separator_;
            } while (level == RIL_BLOCK && res_it.block() == res_it.prev_block());
        }
        int length = text.length() + 1;
        char* result = new char[length];
        strncpy(result, text.string(), length);
        return result;
    }

    char* LTRResultIterator::GetUTF8Text(PageIteratorLevel level) const {
        if (it_->word() == NULL) return NULL; // Already at the end!
        STRING text;
        PAGE_RES_IT res_it(*it_);
        WERD_CHOICE* best_choice = res_it.word()->best_choice;
        ASSERT_HOST(best_choice != NULL);
        if (level == RIL_SYMBOL) {
            text = res_it.word()->BestUTF8(blob_index_, false);
        } else if (level == RIL_WORD) {
            text = best_choice->unichar_string();
        } else {
            bool eol = false; // end of line?
            bool eop = false; // end of paragraph?
            int i = 0;
            do { // for each paragraph in a block
                do { // for each text line in a paragraph
                    do { // for each word in a text line
                        best_choice = res_it.word()->best_choice;
                        ASSERT_HOST(best_choice != NULL);
                        text += best_choice->unichar_string();
                        text += " ";
                        res_it.forward();
                        eol = res_it.row() != res_it.prev_row();
                        i++;
                    } while (!eol);
                    text.truncate_at(text.length() - 1);
                    text += line_separator_;
                    eop = res_it.block() != res_it.prev_block() ||
                            res_it.row()->row->para() != res_it.prev_row()->row->para();
                } while (level != RIL_TEXTLINE && !eop);
                if (eop) text += paragraph_separator_;
            } while (level == RIL_BLOCK && res_it.block() == res_it.prev_block());
        }
        int length = text.length() + 1;
        char* result = new char[length];
        strncpy(result, text.string(), length);
        return result;
    }
    
    // Set the string inserted at the end of each text line. "\n" by default.

    void LTRResultIterator::SetLineSeparator(const char *new_line) {
        line_separator_ = new_line;
    }

    // Set the string inserted at the end of each paragraph. "\n" by default.

    void LTRResultIterator::SetParagraphSeparator(const char *new_para) {
        paragraph_separator_ = new_para;
    }

    // Returns the mean confidence of the current object at the given level.
    // The number should be interpreted as a percent probability. (0.0f-100.0f)

    float LTRResultIterator::Confidence(PageIteratorLevel level) const {
        if (it_->word() == NULL) return 0.0f; // Already at the end!
        float mean_certainty = 0.0f;
        int certainty_count = 0;
        PAGE_RES_IT res_it(*it_);
        WERD_CHOICE* best_choice = res_it.word()->best_choice;
        ASSERT_HOST(best_choice != NULL);
        switch (level) {
            case RIL_BLOCK:
                do {
                    best_choice = res_it.word()->best_choice;
                    ASSERT_HOST(best_choice != NULL);
                    mean_certainty += best_choice->certainty();
                    ++certainty_count;
                    res_it.forward();
                } while (res_it.block() == res_it.prev_block());
                break;
            case RIL_PARA:
                do {
                    best_choice = res_it.word()->best_choice;
                    ASSERT_HOST(best_choice != NULL);
                    mean_certainty += best_choice->certainty();
                    ++certainty_count;
                    res_it.forward();
                } while (res_it.block() == res_it.prev_block() &&
                        res_it.row()->row->para() == res_it.prev_row()->row->para());
                break;
            case RIL_TEXTLINE:
                do {
                    best_choice = res_it.word()->best_choice;
                    ASSERT_HOST(best_choice != NULL);
                    mean_certainty += best_choice->certainty();
                    ++certainty_count;
                    res_it.forward();
                } while (res_it.row() == res_it.prev_row());
                break;
            case RIL_WORD:
                mean_certainty += best_choice->certainty();
                ++certainty_count;
                break;
            case RIL_SYMBOL:
                BLOB_CHOICE_LIST_CLIST* choices = best_choice->blob_choices();
                if (choices != NULL) {
                    BLOB_CHOICE_LIST_C_IT blob_choices_it(choices);
                    for (int blob = 0; blob < blob_index_; ++blob)
                        blob_choices_it.forward();
                    BLOB_CHOICE_IT choice_it(blob_choices_it.data());
                    for (choice_it.mark_cycle_pt();
                            !choice_it.cycled_list();
                            choice_it.forward()) {
                        if (choice_it.data()->unichar_id() ==
                                best_choice->unichar_id(blob_index_))
                            break;
                    }
                    mean_certainty += choice_it.data()->certainty();
                } else {
                    mean_certainty += best_choice->certainty();
                }
                ++certainty_count;
        }
        if (certainty_count > 0) {
            mean_certainty /= certainty_count;
            float confidence = 100 + 5 * mean_certainty;
            if (confidence < 0.0f) confidence = 0.0f;
            if (confidence > 100.0f) confidence = 100.0f;
            return confidence;
        }
        return 0.0f;
    }

    // Returns the font attributes of the current word. If iterating at a higher
    // level object than words, eg textlines, then this will return the
    // attributes of the first word in that textline.
    // The actual return value is a string representing a font name. It points
    // to an internal table and SHOULD NOT BE DELETED. Lifespan is the same as
    // the iterator itself, ie rendered invalid by various members of
    // TessBaseAPI, including Init, SetImage, End or deleting the TessBaseAPI.
    // Pointsize is returned in printers points (1/72 inch.)

    const char* LTRResultIterator::WordFontAttributes(bool* is_bold,
            bool* is_italic,
            bool* is_underlined,
            bool* is_monospace,
            bool* is_serif,
            bool* is_smallcaps,
            int* pointsize,
            int* font_id) const {
        if (it_->word() == NULL) return NULL; // Already at the end!
        if (it_->word()->fontinfo == NULL) {
            *font_id = -1;
            return NULL; // No font information.
        }
        const FontInfo& font_info = *it_->word()->fontinfo;
        *font_id = font_info.universal_id;
        *is_bold = font_info.is_bold();
        *is_italic = font_info.is_italic();
        *is_underlined = false; // TODO(rays) fix this!
        *is_monospace = font_info.is_fixed_pitch();
        *is_serif = font_info.is_serif();
        *is_smallcaps = it_->word()->small_caps;
        float row_height = it_->row()->row->x_height() +
                it_->row()->row->ascenders() - it_->row()->row->descenders();
        // Convert from pixels to printers points.
        *pointsize = scaled_yres_ > 0
                ? static_cast<int> (row_height * kPointsPerInch / scaled_yres_ + 0.5)
                : 0;

        return font_info.name;
    }

    // Returns the name of the language used to recognize this word.

    const char* LTRResultIterator::WordRecognitionLanguage() const {
        if (it_->word() == NULL || it_->word()->tesseract == NULL) return NULL;
        return it_->word()->tesseract->lang.string();
    }

    // Return the overall directionality of this word.

    StrongScriptDirection LTRResultIterator::WordDirection() const {
        if (it_->word() == NULL) return DIR_NEUTRAL;
        bool has_rtl = it_->word()->AnyRtlCharsInWord();
        bool has_ltr = it_->word()->AnyLtrCharsInWord();
        if (has_rtl && !has_ltr)
            return DIR_RIGHT_TO_LEFT;
        if (has_ltr && !has_rtl)
            return DIR_LEFT_TO_RIGHT;
        if (!has_ltr && !has_rtl)
            return DIR_NEUTRAL;
        return DIR_MIX;
    }

    // Returns true if the current word was found in a dictionary.

    bool LTRResultIterator::WordIsFromDictionary() const {
        if (it_->word() == NULL) return false; // Already at the end!
        int permuter = it_->word()->best_choice->permuter();
        return permuter == SYSTEM_DAWG_PERM || permuter == FREQ_DAWG_PERM ||
                permuter == USER_DAWG_PERM;
    }

    // Returns true if the current word is numeric.

    bool LTRResultIterator::WordIsNumeric() const {
        if (it_->word() == NULL) return false; // Already at the end!
        int permuter = it_->word()->best_choice->permuter();
        return permuter == NUMBER_PERM;
    }

    // Returns true if the word contains blamer information.

    bool LTRResultIterator::HasBlamerInfo() const {
        return (it_->word() != NULL && it_->word()->blamer_bundle != NULL &&
                (it_->word()->blamer_bundle->debug.length() > 0 ||
                it_->word()->blamer_bundle->misadaption_debug.length() > 0));
    }

    // Returns the pointer to ParamsTrainingBundle stored in the BlamerBundle
    // of the current word.

    void *LTRResultIterator::GetParamsTrainingBundle() const {
        return (it_->word() != NULL && it_->word()->blamer_bundle != NULL) ?
                &(it_->word()->blamer_bundle->params_training_bundle) : NULL;
    }

    // Returns the pointer to the string with blamer information for this word.
    // Assumes that the word's blamer_bundle is not NULL.

    const char *LTRResultIterator::GetBlamerDebug() const {
        return it_->word()->blamer_bundle->debug.string();
    }

    // Returns the pointer to the string with misadaption information for this word.
    // Assumes that the word's blamer_bundle is not NULL.

    const char *LTRResultIterator::GetBlamerMisadaptionDebug() const {
        return it_->word()->blamer_bundle->misadaption_debug.string();
    }

    // Returns the null terminated UTF-8 encoded truth string for the current word.
    // Use delete [] to free after use.

    char* LTRResultIterator::WordTruthUTF8Text() const {
        if (it_->word() == NULL) return NULL; // Already at the end!
        if (it_->word()->blamer_bundle == NULL ||
                it_->word()->blamer_bundle->incorrect_result_reason == IRR_NO_TRUTH) {
            return NULL; // no truth information for this word
        }
        const GenericVector<STRING> &truth_vec =
                it_->word()->blamer_bundle->truth_text;
        STRING truth_text;
        for (int i = 0; i < truth_vec.size(); ++i) truth_text += truth_vec[i];
        int length = truth_text.length() + 1;
        char* result = new char[length];
        strncpy(result, truth_text.string(), length);
        return result;
    }

    // Returns a pointer to serialized choice lattice.
    // Fills lattice_size with the number of bytes in lattice data.

    const char *LTRResultIterator::WordLattice(int *lattice_size) const {
        if (it_->word() == NULL) return NULL; // Already at the end!
        if (it_->word()->blamer_bundle == NULL) return NULL;
        *lattice_size = it_->word()->blamer_bundle->lattice_size;
        return it_->word()->blamer_bundle->lattice_data;
    }

    // Returns true if the current symbol is a superscript.
    // If iterating at a higher level object than symbols, eg words, then
    // this will return the attributes of the first symbol in that word.

    bool LTRResultIterator::SymbolIsSuperscript() const {
        if (cblob_it_ == NULL && it_->word() != NULL)
            return it_->word()->box_word->BlobPosition(blob_index_) == SP_SUPERSCRIPT;
        return false;
    }

    // Returns true if the current symbol is a subscript.
    // If iterating at a higher level object than symbols, eg words, then
    // this will return the attributes of the first symbol in that word.

    bool LTRResultIterator::SymbolIsSubscript() const {
        if (cblob_it_ == NULL && it_->word() != NULL)
            return it_->word()->box_word->BlobPosition(blob_index_) == SP_SUBSCRIPT;
        return false;
    }

    // Returns true if the current symbol is a dropcap.
    // If iterating at a higher level object than symbols, eg words, then
    // this will return the attributes of the first symbol in that word.

    bool LTRResultIterator::SymbolIsDropcap() const {
        if (cblob_it_ == NULL && it_->word() != NULL)
            return it_->word()->box_word->BlobPosition(blob_index_) == SP_DROPCAP;
        return false;
    }

    ChoiceIterator::ChoiceIterator(const LTRResultIterator& result_it) {
        ASSERT_HOST(result_it.it_->word() != NULL);
        word_res_ = result_it.it_->word();
        PAGE_RES_IT res_it(*result_it.it_);
        WERD_CHOICE* best_choice = word_res_->best_choice;
        BLOB_CHOICE_LIST_CLIST* choices = best_choice->blob_choices();
        if (choices != NULL) {
            BLOB_CHOICE_LIST_C_IT blob_choices_it(choices);
            for (int blob = 0; blob < result_it.blob_index_; ++blob)
                blob_choices_it.forward();
            choice_it_ = new BLOB_CHOICE_IT(blob_choices_it.data());
            choice_it_->mark_cycle_pt();
        } else {
            choice_it_ = NULL;
        }
    }

    ChoiceIterator::~ChoiceIterator() {
        delete choice_it_;
    }

    // Moves to the next choice for the symbol and returns false if there
    // are none left.

    bool ChoiceIterator::Next() {
        if (choice_it_ == NULL)
            return false;
        choice_it_->forward();
        return !choice_it_->cycled_list();
    }

    // Returns the null terminated UTF-8 encoded text string for the current
    // choice. Use delete [] to free after use.

    const char* ChoiceIterator::GetUTF8Text() const {
        if (choice_it_ == NULL)
            return NULL;
        UNICHAR_ID id = choice_it_->data()->unichar_id();
        return word_res_->uch_set->id_to_unichar_ext(id);
    }

    // Returns the confidence of the current choice.
    // The number should be interpreted as a percent probability. (0.0f-100.0f)

    float ChoiceIterator::Confidence() const {
        if (choice_it_ == NULL)
            return 0.0f;
        float confidence = 100 + 5 * choice_it_->data()->certainty();
        if (confidence < 0.0f) confidence = 0.0f;
        if (confidence > 100.0f) confidence = 100.0f;
        return confidence;
    }


} 
// namespace tesseract.
