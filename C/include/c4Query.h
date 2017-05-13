//
//  c4Query.h
//  LiteCore
//
//  Created by Jens Alfke on 10/5/16.
//  Copyright © 2016 Couchbase. All rights reserved.
//

#pragma once
#include "c4Database.h"
#include "c4Document.h"
#include "Fleece.h"

#ifdef __cplusplus
extern "C" {
#endif

    /** \defgroup QueryingDB Querying the Database
        @{ */


    //////// DATABASE QUERIES:


    /** Opaque handle to a compiled query. */
    typedef struct c4Query C4Query;

    
    /** Compiles a query from an expression given as JSON. (The current syntax is based
        on Cloudant's, but this is a placeholder and will be changing.)
        The expression is a predicate that describes which documents should be returned.
        A separate, optional sort expression describes the ordering of the results.

        NOTE: Queries are only supported on SQLite-based databases.
        Queries are currently not supported on databases whose documents use revision trees.
        @param database  The database to be queried.
        @param expression  JSON data describing the query. (Schema is documented elsewhere.)
        @param error  Error will be written here if the function fails.
        @result  A new C4Query, or NULL on failure. */
    C4Query* c4query_new(C4Database *database,
                         C4String expression,
                         C4Error *error) C4API;

    /** Frees a query.  It is legal to pass NULL. */
    void c4query_free(C4Query*) C4API;

    /** Returns a string describing the implementation of the compiled query.
        This is intended to be read by a developer for purposes of optimizing the query, especially
        to add database indexes. */
    C4StringResult c4query_explain(C4Query *query) C4API;


    /** Returns the number of columns (the values specified in the WHAT clause) in each row. */
    unsigned c4query_columnCount(C4Query *query) C4API;

    /** Returns the name of a column. */
    C4StringResult c4query_nameOfColumn(C4Query *query, unsigned col) C4API;


    //////// RUNNING QUERIES:


    /** Options for running queries. */
    typedef struct {
        uint64_t skip;          ///< Number of initial rows to skip
        uint64_t limit;         ///< Max number of rows to return (set to UINT_MAX for unlimited)
        bool rankFullText;      ///< Should full-text results be ranked by relevance?
    } C4QueryOptions;


    /** Default query options. Has skip=0, limit=UINT_MAX, rankFullText=true. */
	CBL_CORE_API extern const C4QueryOptions kC4DefaultQueryOptions;


    /** Info about a match of a full-text query term. */
    typedef struct {
        uint32_t termIndex;                 ///< Index of the search term in the tokenized query
        uint32_t start, length;             ///< *Byte* range of word in query string
    } C4FullTextTerm;


    /** A query result enumerator.
        Created by c4db_query. Must be freed with c4queryenum_free.
        The fields of this struct represent the current matched index row, and are replaced by the
        next call to c4queryenum_next or c4queryenum_free.
        The memory pointed to by slice fields is valid until the enumerator is advanced or freed. */
    typedef struct {
        // All query types:
        C4String docID;                       ///< ID of doc that emitted this row
        C4SequenceNumber docSequence;        ///< Sequence number of doc that emitted row

        // Expression-based only:
        C4String revID;
        C4DocumentFlags docFlags;
        FLArrayIterator columns;

        // Full-text only:
        uint32_t fullTextTermCount;          ///< The number of terms that were matched
        const C4FullTextTerm *fullTextTerms; ///< Array of terms that were matched
    } C4QueryEnumerator;


    /** Runs a compiled query.
        NOTE: Queries will run much faster if the appropriate properties are indexed.
        Indexes must be created explicitly by calling `c4db_createIndex`.
        @param query  The compiled query to run.
        @param options  Query options; only `skip` and `limit` are currently recognized.
        @param encodedParameters  Optional JSON object whose keys correspond to the named
                parameters in the query expression, and values correspond to the values to
                bind. Any unbound parameters will be `null`.
        @param outError  On failure, will be set to the error status.
        @return  An enumerator for reading the rows, or NULL on error. */
    C4QueryEnumerator* c4query_run(C4Query *query,
                                   const C4QueryOptions *options,
                                   C4String encodedParameters,
                                   C4Error *outError) C4API;

    /** In a full-text query enumerator, returns the string that was emitted during indexing that
        contained the search term(s). */
    C4StringResult c4queryenum_fullTextMatched(C4QueryEnumerator *e,
                                              C4Error *outError) C4API;

    /** Given a docID and sequence number from the enumerator, returns the text that was emitted
        during indexing. */
    C4StringResult c4query_fullTextMatched(C4Query *query,
                                          C4String docID,
                                          C4SequenceNumber seq,
                                          C4Error *outError) C4API;

    /** Advances a query enumerator to the next row, populating its fields.
        Returns true on success, false at the end of enumeration or on error. */
    bool c4queryenum_next(C4QueryEnumerator *e,
                          C4Error *outError) C4API;

    /** Checks whether the query results have changed since this enumerator was created;
        if so, returns a new enumerator. Otherwise returns NULL. */
    C4QueryEnumerator* c4queryenum_refresh(C4QueryEnumerator *e,
                                           C4Error *outError) C4API;

    /** Closes an enumerator without freeing it. This is optional, but can be used to free up
        resources if the enumeration has not reached its end, but will not be freed for a while. */
    void c4queryenum_close(C4QueryEnumerator *e) C4API;

    /** Frees a query enumerator. */
    void c4queryenum_free(C4QueryEnumerator *e) C4API;

    /** @} */


    //////// INDEXES:


    /** \name Database Indexes
     @{ */


    /** Types of indexes. */
    typedef C4_ENUM(uint32_t, C4IndexType) {
        kC4ValueIndex,         ///< Regular index of property value
        kC4FullTextIndex,      ///< Full-text index
        kC4GeoIndex,           ///< Geospatial index of GeoJSON values (NOT YET IMPLEMENTED)
    };


    /** Options for indexes; these each apply to specific types of indexes. */
    typedef struct {
        /** Dominant language of text to be indexed; setting this enables word stemming, i.e.
            matching different cases of the same word ("big" and "bigger", for instance.)
            Can be an ISO-639 language code or a lowercase (English) language name; supported
            languages are: da/danish, nl/dutch, en/english, fi/finnish, fr/french, de/german,
            hu/hungarian, it/italian, no/norwegian, pt/portuguese, ro/romanian, ru/russian,
            es/spanish, sv/swedish, tr/turkish.
            If left null, no stemming occurs.*/
        const char *language;

        /** Should diacritical marks (accents) be ignored? Defaults to false.
            Generally this should be left false for non-English text. */
        bool ignoreDiacritics;
    } C4IndexOptions;


    /** Creates a database index, to speed up subsequent queries.

        The index is on one or more expressions, encoded in the same form as in a query. The first
        expression becomes the primary key. These expressions are evaluated for every document in
        the database and stored in the index. The values must be scalars (no arrays or objects),
        although it's OK if they're `missing` in some documents.
     
        An example `expressionsJSON` is `[[".name.first"]]`, to index on the first-name property.

        It is not an error if the index already exists.

        @param database  The database to index.
        @param expressionsJSON  A JSON array of one or more expressions to index; the first is the
                            primary key. Each expression takes the same form as in a query, which
                            means it's a JSON array as well; don't get mixed up by the nesting.
        @param indexType  The type of index (regular, full-text or geospatial.)
        @param indexOptions  Options for the index. If NULL, each option will get a default value.
        @param outError  On failure, will be set to the error status.
        @return  True on success, false on failure. */
    bool c4db_createIndex(C4Database *database,
                          C4String expressionsJSON,
                          C4IndexType indexType,
                          const C4IndexOptions *indexOptions,
                          C4Error *outError) C4API;

    /** Deletes an index that was created by `c4db_createIndex`.
        @param database  The database to index.
        @param expressionsJSON  The same JSON array value used when creating the index.
        @param indexType  The type of the index.
        @param outError  On failure, will be set to the error status.
        @return  True on success, false on failure. */
    bool c4db_deleteIndex(C4Database *database,
                          C4String expressionsJSON,
                          C4IndexType indexType,
                          C4Error *outError) C4API;

    /** @} */

#ifdef __cplusplus
}
#endif
