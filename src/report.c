#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <hpdf.h>
#include "../include/report.h"
#include "../include/database.h"
#include "../include/security.h" 

#define LOG_FILE "data/transactions.log"

// =======================================================================
// PARTIE 1 : STATISTIQUES (TEXTE)

void report_generer_statistiques(int id_compte, char *out_buffer) {
    FILE *f = fopen(LOG_FILE, "rb");
    if (!f) {
        sprintf(out_buffer, "Erreur: Historique introuvable.");
        return;
    }

    Transaction t;
    int nb_ops = 0;
    double total_depots = 0;
    double total_retraits = 0;

    while (fread(&t, sizeof(Transaction), 1, f)) {
        dechiffrer_struct(&t, sizeof(Transaction));
        if (strcasecmp(t.statut, "REUSSIE") != 0) continue;

        bool est_source = (t.id_compte_source == id_compte);
        bool est_dest   = (t.id_compte_destination == id_compte);

        if (est_source || est_dest) {
            nb_ops++;

            if (strcasecmp(t.type_operation, "DEPOT") == 0) {
                if (est_source) total_depots += t.montant;
            }
            else if (strcasecmp(t.type_operation, "RETRAIT") == 0) {
                if (est_source) total_retraits += t.montant;
            }
            else if (strcasecmp(t.type_operation, "TRANSFERT") == 0) {
                if (est_source) total_retraits += t.montant;
                else if (est_dest) total_depots += t.montant;
            }
        }
    }
    fclose(f);

    sprintf(out_buffer, 
        "--- BILAN CLIENT %d ---\n"
        "Operations : %d\n"
        "Entrees (+) : %.2f TND\n"
        "Sorties (-) : %.2f TND\n"
        "Solde Mvt   : %.2f TND\n",
        id_compte, nb_ops, total_depots, total_retraits, 
        (total_depots - total_retraits)
    );
}

// =======================================================================
// PARTIE 2 : GENERATION PDF (AVEC SOLDES AVANT/APRES)

void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data) {
    // Gestion erreur silencieuse
}

bool report_generer_pdf(int id_compte, const char *nom_titulaire, double solde_actuel,char *out_buffer) {
    HPDF_Doc pdf;
    HPDF_Page page;
    HPDF_Font font;
    char buffer[256];
    const int PAGE_HEIGHT = 841; 
    const int PAGE_WIDTH = 595;
    const int MARGIN = 40; // Marge un peu réduite pour faire tenir les colonnes
    int y = PAGE_HEIGHT - MARGIN;

    pdf = HPDF_New(error_handler, NULL);
    if (!pdf) return false;

    HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);
    page = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
    font = HPDF_GetFont(pdf, "Helvetica", NULL);

    HPDF_Image logo = HPDF_LoadPngImageFromFile(pdf, "data/logo.png");

    if (logo) {

        double logo_width = 200; 
        double logo_height = 28; 

        double logo_x = PAGE_WIDTH - MARGIN - logo_width;
        double logo_y = PAGE_HEIGHT - MARGIN - logo_height;

        HPDF_Page_DrawImage(page, logo, logo_x, logo_y, logo_width, logo_height);
    
    }

    // --- EN-TÊTE ---
    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, 18);
    HPDF_Page_TextOut(page, MARGIN, y-24, "HISTORIQUE DE COMPTE");
    y -= 50;

    HPDF_Page_SetFontAndSize(page, font, 10);
    sprintf(buffer, "Titulaire : %s (ID: %d)", nom_titulaire, id_compte);
    HPDF_Page_TextOut(page, MARGIN, y, buffer);
    y -= 15;

    time_t now = time(NULL);
    char date_str[30];
    strftime(date_str, 30, "%d/%m/%Y %H:%M", localtime(&now));
    sprintf(buffer, "Date Edition : %s", date_str);
    HPDF_Page_TextOut(page, MARGIN, y, buffer);
    y -= 30;

    HPDF_Page_SetFontAndSize(page, font, 12);
    sprintf(buffer, "SOLDE ACTUEL : %.2f TND", solde_actuel);
    HPDF_Page_TextOut(page, MARGIN, y, buffer);
    y -= 20;
    HPDF_Page_EndText(page);

    // --- EN-TETES DU TABLEAU ---
    y -= 10;
    HPDF_Page_SetLineWidth(page, 1);
    HPDF_Page_MoveTo(page, MARGIN, y);
    HPDF_Page_LineTo(page, 550, y);
    HPDF_Page_Stroke(page);
    y -= 15;

    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, font, 9);
    
    // Positionnement des colonnes (x)
    int x_date = MARGIN;
    int x_ref  = MARGIN + 80;
    int x_type = MARGIN + 140;
    int x_deb  = MARGIN + 220; // Débit
    int x_cred = MARGIN + 300; // Crédit
    int x_sold = MARGIN + 380; // Solde Résultant

    HPDF_Page_TextOut(page, x_date, y, "DATE");
    HPDF_Page_TextOut(page, x_ref, y, "REF.");
    HPDF_Page_TextOut(page, x_type, y, "OPERATION");
    HPDF_Page_TextOut(page, x_deb, y, "DEBIT (-)");
    HPDF_Page_TextOut(page, x_cred, y, "CREDIT (+)");
    HPDF_Page_TextOut(page, x_sold, y, "SOLDE");
    HPDF_Page_EndText(page);

    y -= 5;
    HPDF_Page_MoveTo(page, MARGIN, y);
    HPDF_Page_LineTo(page, 550, y);
    HPDF_Page_Stroke(page);
    y -= 15;

    // --- LECTURE LOGS ---
    FILE *f = fopen(LOG_FILE, "rb");
    if (f) {
        Transaction t;
        HPDF_Page_BeginText(page);
        HPDF_Page_SetFontAndSize(page, font, 9);

        while (fread(&t, sizeof(Transaction), 1, f)) {
            dechiffrer_struct(&t, sizeof(Transaction));
            bool est_source = (t.id_compte_source == id_compte);
            bool est_dest   = (t.id_compte_destination == id_compte);
            bool est_effectuee = (strcasecmp(t.statut, "REUSSIE") == 0);

            if ((est_source || est_dest )&& est_effectuee) {
                // Gestion saut de page
                if (y < MARGIN) {
                    HPDF_Page_EndText(page);
                    page = HPDF_AddPage(pdf);
                    HPDF_Page_SetFontAndSize(page, font, 9);
                    HPDF_Page_BeginText(page);
                    y = PAGE_HEIGHT - MARGIN;
                }

                // 1. Date
                strftime(date_str, 30, "%d/%m/%Y", localtime(&t.date_transaction));
                HPDF_Page_TextOut(page, x_date, y, date_str);

                // 2. Ref (ID Transaction)
                sprintf(buffer, "#%d", t.id_transaction);
                HPDF_Page_TextOut(page, x_ref, y, buffer);

                // 3. Type
                HPDF_Page_TextOut(page, x_type, y, t.type_operation);

                // 4. Montants (Débit / Crédit) et Solde
                double solde_resultant = 0.0;

                if (strcmp(t.type_operation, "TRANSFERT") == 0) {
                    if (est_source) {
                        // J'ai envoyé : Débit
                        sprintf(buffer, "%.2f", t.montant);
                        HPDF_Page_TextOut(page, x_deb, y, buffer);
                        solde_resultant = t.solde_apres; // Mon solde après envoi
                    } else {
                        // J'ai reçu : Crédit
                        sprintf(buffer, "%.2f", t.montant);
                        HPDF_Page_TextOut(page, x_cred, y, buffer);
                        // Pour la destination, on n'a pas stocké le "solde après" dans CETTE ligne de log unique
                        // On affiche "-" par défaut.
                        solde_resultant = 0.0; 
                    }
                } 
                else if (strcmp(t.type_operation, "RETRAIT") == 0) {
                     sprintf(buffer, "%.2f", t.montant);
                     HPDF_Page_TextOut(page, x_deb, y, buffer);
                     solde_resultant = t.solde_apres;
                }
                else { // DEPOT
                     sprintf(buffer, "%.2f", t.montant);
                     HPDF_Page_TextOut(page, x_cred, y, buffer);
                     solde_resultant = t.solde_apres;
                }

                // 5. Affichage Solde
                if (solde_resultant > 0.001) {
                    sprintf(buffer, "%.2f", solde_resultant);
                    HPDF_Page_TextOut(page, x_sold, y, buffer);
                } else {
                    HPDF_Page_TextOut(page, x_sold, y, "-");
                }

                y -= 12;
            }
        }
        HPDF_Page_EndText(page);
        fclose(f);
    }

    char filename[100];
    strftime(date_str, 30, "%d_%m_%Y_%H_%M", localtime(&now));
    sprintf(filename, "data/pdf/releve_%d_%s.pdf", id_compte, date_str);
    HPDF_SaveToFile(pdf, filename);
    HPDF_Free(pdf);
    sprintf(out_buffer, "%s", filename);
    return true;
}