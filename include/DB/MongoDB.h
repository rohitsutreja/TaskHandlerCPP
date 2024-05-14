#pragma once

#include <bsoncxx/document/view.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/json.hpp>


class MongoDB
{
private:
	mongocxx::instance inst{};
	mongocxx::client client;
	mongocxx::database db;

public:

	MongoDB()
		: inst{}
		, client{ mongocxx::uri{ "mongodb+srv://rohit:drbezhkeTNd7rEZL@cluster0.ldjcnlk.mongodb.net/?retryWrites=true&w=majority&appName=Cluster0" } }
		, db {client["test"]} {}

	auto& getDB() {
		return db;
	}

	static std::shared_ptr<MongoDB> getInstance() {
		static auto ptr =  std::make_shared<MongoDB>();
		return ptr;
	}
};

