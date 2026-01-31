# Mainstreet Metrics
Mainstreet Metrics is a standalone C++ console application designed to help community members discover and support local small businesses. This tool provides and interactive interface for users to browse, review, and bookmark local favorites while securing user data through hashing and bot prevention measures. It also allows business owners to do a CSV export and has a simple, direction based UI.

# Features
To meet the 2025-2026 FBLA requirements, the application includes features such as:
* Business Discovery: Browse a database of local businessess categorized by Food, Retail, and Services.
* Dynamic Sorting: Sort businessess based on categories or their community-sourced ratings and reviews.
* Review System: Users can contribute to the community by leaving their own ratings and reviews.
* Favorites: The bookmarking feature allows users to svae specific businessess for easy access later.
* Special Deals: A display system designed around special coupons and local deals.
* Security Verification: Includes a simple bot-prevention captcha to ensure all intereactions are from humans.

# Application Architecture:
It follows a layered architecture:

UI Layer (Menus, Tables, ANSI)
Application Logic (Validation, Distance, Sorting, Export)
BusinessManager API (Users, Businesses, Reviews, Bookmarks)
MongoDB Backend
