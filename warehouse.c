#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Forward declarations
typedef struct PAGE PAGE;
typedef struct SHEET SHEET;
typedef struct PRODUCT PRODUCT;
typedef struct DATABASE DATABASE;

// Observer callback type
typedef void (*ObserverCallback)(const char*, void*);

// RECORD structure
typedef struct RECORD {
    double input;
    double output;
    double sold_final;
    double sold_init;
    char* doc_id;
    char* doc_type;
    int dom;
    PAGE* page;
} RECORD;

// PAGE structure
typedef struct PAGE {
    double price;
    double sold_init;
    double sold_final;
    int crtrecord;
    int maxrecord;
    RECORD** records;
    SHEET* sheet;
} PAGE;

// SHEET structure
typedef struct SHEET {
    int year;
    int month;
    int crtpage;
    int maxpage;
    PAGE** pages;
    PRODUCT* product;
} SHEET;

// PRODUCT structure
typedef struct PRODUCT {
    char* name;
    char* unit;
    int crtsheet;
    int maxsheet;
    SHEET** sheets;
    DATABASE* database;
} PRODUCT;

// DATABASE structure
typedef struct DATABASE {
    int crtproduct;
    int maxproduct;
    PRODUCT** products;
    ObserverCallback* observers;
    int observer_count;
} DATABASE;

// Function prototypes
RECORD* record_init(RECORD* self);
RECORD* record_init_values(RECORD* self, double input_, double output_, double sold_final_, 
                         double sold_init_, const char* doc_id_, const char* doc_type_, int dom_);
void record_update_values(RECORD* self, double input_, double output_, 
                        const char* doc_id_, const char* doc_type_, int dom_);

PAGE* page_init(PAGE* self);
PAGE* page_init_values(PAGE* self, double price_, double sold_init_);
RECORD* page_add_record(PAGE* self, RECORD* record);
void page_update_totals(PAGE* self);
RECORD* page_create_record(PAGE* self, double input_, double output_,
                          const char* doc_id_, const char* doc_type_, int dom_);
bool page_select_record(PAGE* self, int index);
bool page_remove_record(PAGE* self, int index);
void page_recalculate_records(PAGE* self);

SHEET* sheet_init(SHEET* self);
SHEET* sheet_init_values(SHEET* self, int year_, int month_);
PAGE* sheet_create_page(SHEET* self, double price, double sold_init);
void sheet_update_totals(SHEET* self);
bool sheet_select_page(SHEET* self, int index);
bool sheet_remove_page(SHEET* self, int index);

PRODUCT* product_init(PRODUCT* self);
PRODUCT* product_init_values(PRODUCT* self, const char* name_, const char* unit_);
SHEET* product_create_sheet(PRODUCT* self, int year, int month);
void product_update_totals(PRODUCT* self);
bool product_select_sheet(PRODUCT* self, int index);
bool product_remove_sheet(PRODUCT* self, int index);
void product_update_info(PRODUCT* self, const char* name_, const char* unit_);

DATABASE* database_init(DATABASE* self);
PRODUCT* database_create_product(DATABASE* self, const char* name, const char* unit);
bool database_select_product(DATABASE* self, int index);
bool database_remove_product(DATABASE* self, int index);
void database_add_observer(DATABASE* self, ObserverCallback callback);
void database_remove_observer(DATABASE* self, ObserverCallback callback);
void database_notify_change(DATABASE* self, const char* event_type, void* data);
PRODUCT* database_get_current_product(DATABASE* self);
SHEET* database_get_current_sheet(DATABASE* self);
PAGE* database_get_current_page(DATABASE* self);
RECORD* database_get_current_record(DATABASE* self);

// Example observer callback
void ui_update_handler(const char* event_type, void* data);

// Global database instance
DATABASE db;

// RECORD implementation
RECORD* record_init(RECORD* self) {
    self->input = 0.0;
    self->output = 0.0;
    self->sold_final = 0.0;
    self->sold_init = 0.0;
    self->doc_id = strdup("");
    self->doc_type = strdup("");
    self->dom = 0;
    self->page = NULL;
    return self;
}

RECORD* record_init_values(RECORD* self, double input_, double output_, double sold_final_, 
                         double sold_init_, const char* doc_id_, const char* doc_type_, int dom_) {
    self->input = input_;
    self->output = output_;
    self->sold_init = sold_init_;
    free(self->doc_id);
    self->doc_id = strdup(doc_id_);
    free(self->doc_type);
    self->doc_type = strdup(doc_type_);
    self->dom = dom_;
    self->sold_final = self->sold_init + self->input - self->output;

    if (self->page) {
        page_update_totals(self->page);
    }

    return self;
}

void record_update_values(RECORD* self, double input_, double output_, 
                        const char* doc_id_, const char* doc_type_, int dom_) {
    if (input_ != -1.0) {  // Using -1.0 as "None" equivalent
        self->input = input_;
    }
    if (output_ != -1.0) {
        self->output = output_;
    }
    if (doc_id_) {
        free(self->doc_id);
        self->doc_id = strdup(doc_id_);
    }
    if (doc_type_) {
        free(self->doc_type);
        self->doc_type = strdup(doc_type_);
    }
    if (dom_ != -1) {
        self->dom = dom_;
    }

    self->sold_final = self->sold_init + self->input - self->output;

    if (self->page) {
        page_update_totals(self->page);
    }
}

// PAGE implementation
PAGE* page_init(PAGE* self) {
    self->price = 0.0;
    self->sold_init = 0.0;
    self->sold_final = 0.0;
    self->crtrecord = 0;
    self->maxrecord = 0;
    self->records = NULL;
    self->sheet = NULL;
    return self;
}

PAGE* page_init_values(PAGE* self, double price_, double sold_init_) {
    self->price = price_;
    self->sold_init = sold_init_;
    self->sold_final = sold_init_;
    self->crtrecord = 0;
    self->maxrecord = 0;
    return self;
}

RECORD* page_add_record(PAGE* self, RECORD* record) {
    record->page = self;
    self->maxrecord++;
    self->records = realloc(self->records, self->maxrecord * sizeof(RECORD*));
    self->records[self->maxrecord - 1] = record;
    self->crtrecord = self->maxrecord - 1;

    page_update_totals(self);
    return record;
}

void page_update_totals(PAGE* self) {
    if (self->maxrecord > 0) {
        self->sold_final = self->records[self->maxrecord - 1]->sold_final;
    } else {
        self->sold_final = self->sold_init;
    }

    if (self->sheet) {
        sheet_update_totals(self->sheet);
    }
}

RECORD* page_create_record(PAGE* self, double input_, double output_,
                          const char* doc_id_, const char* doc_type_, int dom_) {
    RECORD* new_record = malloc(sizeof(RECORD));
    record_init(new_record);
    
    double current_sold_init = (self->maxrecord == 0) ? self->sold_init : self->sold_final;
    record_init_values(new_record, input_, output_, 0.0, current_sold_init, doc_id_, doc_type_, dom_);
    
    return page_add_record(self, new_record);
}

bool page_select_record(PAGE* self, int index) {
    if (index >= 0 && index < self->maxrecord) {
        self->crtrecord = index;
        return true;
    }
    return false;
}

bool page_remove_record(PAGE* self, int index) {
    if (index >= 0 && index < self->maxrecord) {
        free(self->records[index]->doc_id);
        free(self->records[index]->doc_type);
        free(self->records[index]);
        
        for (int i = index; i < self->maxrecord - 1; i++) {
            self->records[i] = self->records[i + 1];
        }
        
        self->maxrecord--;
        self->records = realloc(self->records, self->maxrecord * sizeof(RECORD*));
        
        if (self->crtrecord >= self->maxrecord && self->maxrecord > 0) {
            self->crtrecord = self->maxrecord - 1;
        } else if (self->maxrecord == 0) {
            self->crtrecord = 0;
        }

        page_recalculate_records(self);
        page_update_totals(self);
        return true;
    }
    return false;
}

void page_recalculate_records(PAGE* self) {
    double current_sold = self->sold_init;
    for (int i = 0; i < self->maxrecord; i++) {
        self->records[i]->sold_init = current_sold;
        self->records[i]->sold_final = self->records[i]->sold_init + self->records[i]->input - self->records[i]->output;
        current_sold = self->records[i]->sold_final;
    }
}

// SHEET implementation
SHEET* sheet_init(SHEET* self) {
    self->year = 2025;
    self->month = 1;
    self->crtpage = 0;
    self->maxpage = 0;
    self->pages = NULL;
    self->product = NULL;
    return self;
}

SHEET* sheet_init_values(SHEET* self, int year_, int month_) {
    self->year = year_;
    self->month = month_;
    self->crtpage = 0;
    self->maxpage = 0;
    return self;
}

PAGE* sheet_create_page(SHEET* self, double price, double sold_init) {
    PAGE* new_page = malloc(sizeof(PAGE));
    page_init(new_page);
    new_page->sheet = self;
    page_init_values(new_page, price, sold_init);
    
    self->maxpage++;
    self->pages = realloc(self->pages, self->maxpage * sizeof(PAGE*));
    self->pages[self->maxpage - 1] = new_page;
    self->crtpage = self->maxpage - 1;

    sheet_update_totals(self);
    return new_page;
}

void sheet_update_totals(SHEET* self) {
    if (self->product) {
        product_update_totals(self->product);
    }
}

bool sheet_select_page(SHEET* self, int index) {
    if (index >= 0 && index < self->maxpage) {
        self->crtpage = index;
        return true;
    }
    return false;
}

bool sheet_remove_page(SHEET* self, int index) {
    if (index >= 0 && index < self->maxpage) {
        // Free all records in the page first
        for (int i = 0; i < self->pages[index]->maxrecord; i++) {
            free(self->pages[index]->records[i]->doc_id);
            free(self->pages[index]->records[i]->doc_type);
            free(self->pages[index]->records[i]);
        }
        free(self->pages[index]->records);
        free(self->pages[index]);
        
        for (int i = index; i < self->maxpage - 1; i++) {
            self->pages[i] = self->pages[i + 1];
        }
        
        self->maxpage--;
        self->pages = realloc(self->pages, self->maxpage * sizeof(PAGE*));
        
        if (self->crtpage >= self->maxpage && self->maxpage > 0) {
            self->crtpage = self->maxpage - 1;
        } else if (self->maxpage == 0) {
            self->crtpage = 0;
        }

        sheet_update_totals(self);
        return true;
    }
    return false;
}

// PRODUCT implementation
PRODUCT* product_init(PRODUCT* self) {
    self->name = strdup("");
    self->unit = strdup("");
    self->crtsheet = 0;
    self->maxsheet = 0;
    self->sheets = NULL;
    self->database = NULL;
    return self;
}

PRODUCT* product_init_values(PRODUCT* self, const char* name_, const char* unit_) {
    free(self->name);
    self->name = strdup(name_);
    free(self->unit);
    self->unit = strdup(unit_);
    self->crtsheet = 0;
    self->maxsheet = 0;
    return self;
}

SHEET* product_create_sheet(PRODUCT* self, int year, int month) {
    SHEET* new_sheet = malloc(sizeof(SHEET));
    sheet_init(new_sheet);
    new_sheet->product = self;
    sheet_init_values(new_sheet, year, month);
    
    self->maxsheet++;
    self->sheets = realloc(self->sheets, self->maxsheet * sizeof(SHEET*));
    self->sheets[self->maxsheet - 1] = new_sheet;
    self->crtsheet = self->maxsheet - 1;

    product_update_totals(self);
    return new_sheet;
}

void product_update_totals(PRODUCT* self) {
    if (self->database) {
        database_notify_change(self->database, "product_updated", self);
    }
}

bool product_select_sheet(PRODUCT* self, int index) {
    if (index >= 0 && index < self->maxsheet) {
        self->crtsheet = index;
        return true;
    }
    return false;
}

bool product_remove_sheet(PRODUCT* self, int index) {
    if (index >= 0 && index < self->maxsheet) {
        // Free all pages and their records first
        for (int i = 0; i < self->sheets[index]->maxpage; i++) {
            for (int j = 0; j < self->sheets[index]->pages[i]->maxrecord; j++) {
                free(self->sheets[index]->pages[i]->records[j]->doc_id);
                free(self->sheets[index]->pages[i]->records[j]->doc_type);
                free(self->sheets[index]->pages[i]->records[j]);
            }
            free(self->sheets[index]->pages[i]->records);
            free(self->sheets[index]->pages[i]);
        }
        free(self->sheets[index]->pages);
        free(self->sheets[index]);
        
        for (int i = index; i < self->maxsheet - 1; i++) {
            self->sheets[i] = self->sheets[i + 1];
        }
        
        self->maxsheet--;
        self->sheets = realloc(self->sheets, self->maxsheet * sizeof(SHEET*));
        
        if (self->crtsheet >= self->maxsheet && self->maxsheet > 0) {
            self->crtsheet = self->maxsheet - 1;
        } else if (self->maxsheet == 0) {
            self->crtsheet = 0;
        }

        product_update_totals(self);
        return true;
    }
    return false;
}

void product_update_info(PRODUCT* self, const char* name_, const char* unit_) {
    if (name_) {
        free(self->name);
        self->name = strdup(name_);
    }
    if (unit_) {
        free(self->unit);
        self->unit = strdup(unit_);
    }

    if (self->database) {
        database_notify_change(self->database, "product_updated", self);
    }
}

// DATABASE implementation
DATABASE* database_init(DATABASE* self) {
    self->crtproduct = 0;
    self->maxproduct = 0;
    self->products = NULL;
    self->observers = NULL;
    self->observer_count = 0;
    return self;
}

PRODUCT* database_create_product(DATABASE* self, const char* name, const char* unit) {
    PRODUCT* new_product = malloc(sizeof(PRODUCT));
    product_init(new_product);
    new_product->database = self;
    product_init_values(new_product, name, unit);
    
    self->maxproduct++;
    self->products = realloc(self->products, self->maxproduct * sizeof(PRODUCT*));
    self->products[self->maxproduct - 1] = new_product;
    self->crtproduct = self->maxproduct - 1;

    database_notify_change(self, "product_added", new_product);
    return new_product;
}

bool database_select_product(DATABASE* self, int index) {
    if (index >= 0 && index < self->maxproduct) {
        self->crtproduct = index;
        database_notify_change(self, "product_selected", self->products[index]);
        return true;
    }
    return false;
}

bool database_remove_product(DATABASE* self, int index) {
    if (index >= 0 && index < self->maxproduct) {
        PRODUCT* removed_product = self->products[index];
        
        // Free all sheets, pages and records first
        for (int i = 0; i < removed_product->maxsheet; i++) {
            for (int j = 0; j < removed_product->sheets[i]->maxpage; j++) {
                for (int k = 0; k < removed_product->sheets[i]->pages[j]->maxrecord; k++) {
                    free(removed_product->sheets[i]->pages[j]->records[k]->doc_id);
                    free(removed_product->sheets[i]->pages[j]->records[k]->doc_type);
                    free(removed_product->sheets[i]->pages[j]->records[k]);
                }
                free(removed_product->sheets[i]->pages[j]->records);
                free(removed_product->sheets[i]->pages[j]);
            }
            free(removed_product->sheets[i]->pages);
            free(removed_product->sheets[i]);
        }
        free(removed_product->sheets);
        free(removed_product->name);
        free(removed_product->unit);
        
        for (int i = index; i < self->maxproduct - 1; i++) {
            self->products[i] = self->products[i + 1];
        }
        
        self->maxproduct--;
        self->products = realloc(self->products, self->maxproduct * sizeof(PRODUCT*));
        
        if (self->crtproduct >= self->maxproduct && self->maxproduct > 0) {
            self->crtproduct = self->maxproduct - 1;
        } else if (self->maxproduct == 0) {
            self->crtproduct = 0;
        }

        database_notify_change(self, "product_removed", removed_product);
        free(removed_product);
        return true;
    }
    return false;
}

void database_add_observer(DATABASE* self, ObserverCallback callback) {
    self->observer_count++;
    self->observers = realloc(self->observers, self->observer_count * sizeof(ObserverCallback));
    self->observers[self->observer_count - 1] = callback;
}

void database_remove_observer(DATABASE* self, ObserverCallback callback) {
    for (int i = 0; i < self->observer_count; i++) {
        if (self->observers[i] == callback) {
            for (int j = i; j < self->observer_count - 1; j++) {
                self->observers[j] = self->observers[j + 1];
            }
            self->observer_count--;
            self->observers = realloc(self->observers, self->observer_count * sizeof(ObserverCallback));
            break;
        }
    }
}

void database_notify_change(DATABASE* self, const char* event_type, void* data) {
    for (int i = 0; i < self->observer_count; i++) {
        self->observers[i](event_type, data);
    }
}

PRODUCT* database_get_current_product(DATABASE* self) {
    if (self->crtproduct >= 0 && self->crtproduct < self->maxproduct) {
        return self->products[self->crtproduct];
    }
    return NULL;
}

SHEET* database_get_current_sheet(DATABASE* self) {
    PRODUCT* product = database_get_current_product(self);
    if (product && product->crtsheet >= 0 && product->crtsheet < product->maxsheet) {
        return product->sheets[product->crtsheet];
    }
    return NULL;
}

PAGE* database_get_current_page(DATABASE* self) {
    SHEET* sheet = database_get_current_sheet(self);
    if (sheet && sheet->crtpage >= 0 && sheet->crtpage < sheet->maxpage) {
        return sheet->pages[sheet->crtpage];
    }
    return NULL;
}

RECORD* database_get_current_record(DATABASE* self) {
    PAGE* page = database_get_current_page(self);
    if (page && page->crtrecord >= 0 && page->crtrecord < page->maxrecord) {
        return page->records[page->crtrecord];
    }
    return NULL;
}

// Example observer callback implementation
void ui_update_handler(const char* event_type, void* data) {
    printf("UI Update: %s - %p\n", event_type, data);
    // Here you would update your UI components
}

int main() {
    // Initialize database
    database_init(&db);
    
    // Register the UI handler
    database_add_observer(&db, ui_update_handler);
    
    // Example usage
    PRODUCT* p = database_create_product(&db, "Widget", "pieces");
    SHEET* s = product_create_sheet(p, 2025, 7);
    PAGE* pg = sheet_create_page(s, 10.99, 100.0);
    RECORD* r = page_create_record(pg, 50.0, 20.0, "INV001", "invoice", 1);
    
    // Cleanup (not strictly needed here as it's a global)
    // In a real program, you'd want to properly free everything
    return 0;
}