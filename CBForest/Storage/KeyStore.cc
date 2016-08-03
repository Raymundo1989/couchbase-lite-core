//
//  KeyStore.cc
//  CBNano
//
//  Created by Jens Alfke on 11/12/14.
//  Copyright (c) 2014 Couchbase. All rights reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
//  Unless required by applicable law or agreed to in writing, software distributed under the
//  License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
//  either express or implied. See the License for the specific language governing permissions
//  and limitations under the License.

#include "KeyStore.hh"
#include "Document.hh"
#include "Database.hh"
#include "LogInternal.hh"

using namespace std;


namespace cbforest {

    const KeyStore::Options KeyStore::Options::defaults = KeyStore::Options{false, false};

    Document KeyStore::get(slice key, ContentOptions options) const {
        Document doc(key);
        read(doc, options);
        return doc;
    }

    void KeyStore::get(slice key, ContentOptions options, function<void(const Document&)> fn) {
        // Subclasses can implement this differently for better memory management.
        Document doc(key);
        read(doc, options);
        fn(doc);
    }

    void KeyStore::get(sequence seq, ContentOptions options, function<void(const Document&)> fn) {
        fn(get(seq, options));
    }

    void KeyStore::readBody(Document &doc) const {
        Document fullDoc = get(doc.key(), kDefaultContent);
        doc._body = fullDoc._body;  // avoid copying block
    }

    void KeyStore::deleteKeyStore(Transaction& trans) {
        trans.database().deleteKeyStore(name());
    }

    void KeyStore::write(Document &doc, Transaction &t) {
        if (doc.deleted()) {
            del(doc, t);
        } else {
            sequence seq = set(doc.key(), doc.meta(), doc.body(), t);
            updateDoc(doc, seq);
        }
    }

    bool KeyStore::del(slice key, Transaction &t) {
        bool ok = _del(key, t);
        if (ok)
            t.incrementDeletionCount();
        return ok;
    }

    bool KeyStore::del(sequence s, Transaction &t) {
        bool ok = _del(s, t);
        if (ok)
            t.incrementDeletionCount();
        return ok;
    }

    bool KeyStore::del(cbforest::Document &doc, Transaction &t) {
        return doc.sequence() ? del(doc.sequence(), t) : del(doc.key(), t);
    }

}
