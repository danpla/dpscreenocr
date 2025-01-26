#pragma once

#include <stdexcept>
#include <string>


namespace dpso {


class Sha256FileError : public std::runtime_error {
    using runtime_error::runtime_error;
};


extern const char* const sha256FileExt;


// Throws Sha256FileError.
std::string calcFileSha256(const char* filePath);


// Save the SHA-256 hex digest of the digestSourceFilePath file to the
// associated ".sha256" file (the path of this file is
// digestSourceFilePath + ".sha256").
// Throws Sha256FileError.
void saveSha256File(
    const char* digestSourceFilePath, const char* digest);


// Load the SHA-256 hex digest previously saved by saveSha256File().
// Returns an empty string if the ".sha256" file for the given file
// does not exist.
// Throws Sha256FileError.
std::string loadSha256File(const char* digestSourceFilePath);


// Does nothing if the ".sha256" file for the given file does not
// exist.
// Throws Sha256FileError.
void removeSha256File(const char* digestSourceFilePath);


// Get the SHA-256 hex digest of the file from its ".sha256" file,
// creating ".sha256" if it doesn't exist.
//
// The function first tries to load the digest with loadSha256File().
// If the digest file does not exist, the digest is calculated with
// calcFileSha256(), saved with saveSha256File(), and returned.
//
// Throws Sha256FileError.
std::string getSha256HexDigestWithCaching(const char* filePath);


}
