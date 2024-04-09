#pragma once

#include <string>
#include <vector>
#include"crow_all.h"
#include <chrono>
#include"Util.h"
#include "../DB/MongoDB.h"
#include <optional>

//extern mongocxx::database db;



class User {

public:


	User(std::string name, std::string password, std::vector<std::string> roles, bool active = true) : 
		m_username{ name }, m_password{ password }, m_roles{ roles }, m_active{active} {
	}

	User(std::string_view id) : m_id{ id } {

	}

	const std::string& getUserName() const{
		return m_username;
	}

	const std::string& getPassword() const {
		return m_password;
	}

	const std::vector<std::string>& getRoles() {
		return m_roles;
	}

	bool getStatus() {
		return m_active;
	}

	const std::string& getId() const {
		return m_id;
	}


	void setId(std::string_view id) {
		m_id = id;
	}

	void setPassword(std::string password) {
		m_password = password;
	}

	void setUserName(std::string_view username) {
		m_username = username;
	}

	void setStatus(bool active) {
		m_active = active;
	}

	void setRoles(std::vector<std::string> roles) {
		m_roles = roles;
	}

	void setCreatedAt(std::string_view ca) {
		m_createdAt = ca;
	}

	void setUpdatedAt(std::string_view ua) {
		m_updatedAt = ua;
	}



	friend bool operator==(const User& u1, const User& u2) {
		return u1.getId() == u2.getId();
	}


	bool save() {

		//mongocxx::collection coll = db["users"];
		mongocxx::collection coll = MongoDB::getInstance()->getDB()["users"];

		bsoncxx::builder::basic::array roles_array;

		for (const auto& role : m_roles) {
			roles_array.append(role);
		}

		auto timestamp = getLogTimeString();

		auto document = bsoncxx::builder::basic::make_document(
			bsoncxx::builder::basic::kvp("username", m_username),
			bsoncxx::builder::basic::kvp("password", m_password),
			bsoncxx::builder::basic::kvp("roles", roles_array),
			bsoncxx::builder::basic::kvp("isActive", m_active),
			bsoncxx::builder::basic::kvp("createdAt", timestamp),
			bsoncxx::builder::basic::kvp("updatedAt", timestamp)
			
		);

		// Convert document to JSON for debugging
		std::cout << bsoncxx::to_json(document.view()) << std::endl;

		// Insert the document into the "users" collection
		auto result = coll.insert_one(document.view());

		if (result) {
	
			auto idElement = result->inserted_id();
			bsoncxx::types::b_oid id = idElement.get_oid();

			// Convert the _id to a string
			m_id = id.value.to_string();
		}
		else {
			return false;
		}

		return true;
	}

	void update() {
		//mongocxx::collection coll = db["users"];
		mongocxx::collection coll = MongoDB::getInstance()->getDB()["users"];
		auto filter = bsoncxx::builder::basic::make_document(
			bsoncxx::builder::basic::kvp("_id", bsoncxx::oid(m_id))
		);


		bsoncxx::builder::basic::array roles_array;

		for (const auto& role : m_roles) {
			roles_array.append(role);
		}

		auto timestamp = getLogTimeString();

		auto update = bsoncxx::builder::basic::make_document(
			bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(
				bsoncxx::builder::basic::kvp("username", m_username),
				bsoncxx::builder::basic::kvp("password", m_password),
				bsoncxx::builder::basic::kvp("roles",roles_array),
				bsoncxx::builder::basic::kvp("isActive", m_active),
				bsoncxx::builder::basic::kvp("updatedAt", timestamp)

			))
		);

		std::cout << bsoncxx::to_json(update.view()) << std::endl;
		coll.update_one(filter.view(), update.view());
	}

	void remove() {
		//mongocxx::collection coll = db["users"];
		mongocxx::collection coll = MongoDB::getInstance()->getDB()["users"];
		auto filter = bsoncxx::builder::basic::make_document(
			bsoncxx::builder::basic::kvp("_id", bsoncxx::oid(m_id))
		);

		coll.delete_one(filter.view());
	}

	static std::optional<User> getUser(std::string id) {
	//	mongocxx::collection coll = db["users"];
		mongocxx::collection coll = MongoDB::getInstance()->getDB()["users"];

		auto id1 = bsoncxx::oid(id);
		auto document = bsoncxx::builder::basic::make_document(
			bsoncxx::builder::basic::kvp("_id", id1)
		);

		auto result = coll.find_one(document.view());

		if (result) {
			bsoncxx::document::view view = result->view();

			std::string name = static_cast<std::string>(view["username"].get_string().value);
			std::string password = static_cast<std::string>(view["password"].get_string().value);
			bool isActive = true;
			std::string createdAt = "";
			std::string updatedAt = "";
			try {
				isActive = view["isActive"].get_bool().value;
				std::string createdAt = static_cast<std::string>(view["createdAt"].get_string().value);
				std::string updatedAt = static_cast<std::string>(view["updatedAt"].get_string().value);
			}
			catch (...) {

			}

			std::vector<std::string> roles;
			auto rolesArray = view["roles"].get_array().value;
			for (const auto& role : rolesArray) {
				roles.push_back(static_cast<std::string>(role.get_string().value));
			}

			User user = User(name, password, roles, isActive);
			user.setId(id);
			user.setCreatedAt(createdAt);
			user.setUpdatedAt(updatedAt);

			return user;
		}

		// Return a default-constructed User if not found
		return {};
	}

	static std::optional<User> getUserByUserName(std::string username) {
		//mongocxx::collection coll = db["users"];
		mongocxx::collection coll = MongoDB::getInstance()->getDB()["users"];
		auto document = bsoncxx::builder::basic::make_document(
			bsoncxx::builder::basic::kvp("username", username)
		);

		auto result = coll.find_one(document.view());

		if (result) {
			bsoncxx::document::view view = result->view();

			std::string id = view["_id"].get_oid().value.to_string();
			std::string name = static_cast<std::string>(view["username"].get_string().value);
			std::string password = static_cast<std::string>(view["password"].get_string().value);
			bool isActive = true;
			std::string createdAt = "";
			std::string updatedAt = "";
			try {
				isActive = view["isActive"].get_bool().value;
				createdAt = static_cast<std::string>(view["createdAt"].get_string().value);
				updatedAt = static_cast<std::string>(view["updatedAt"].get_string().value);
			}
			catch (...) {

			}

			std::vector<std::string> roles;
			auto rolesArray = view["roles"].get_array().value;
			for (const auto& role : rolesArray) {
				roles.push_back(static_cast<std::string>(role.get_string().value));
			}

			User user = User( name, password, roles, isActive);
			user.setId(id);
			user.setCreatedAt(createdAt);
			user.setUpdatedAt(updatedAt);

			return user;
		}

		// Return a default-constructed User if not found
		return {};
	}

	static std::vector<User> getAllUsers() {
	//	mongocxx::collection coll = db["users"];
		mongocxx::collection coll = MongoDB::getInstance()->getDB()["users"];
		auto result = coll.find({});

		std::vector<User> users{};

		for (auto& user : result) {
			std::string id = user["_id"].get_oid().value.to_string();
			std::string name = static_cast<std::string>(user["username"].get_string().value);
			std::string password = static_cast<std::string>(user["password"].get_string().value);
			bool isActive = true;
			std::string createdAt = "";
			std::string updatedAt = "";
			try {
				createdAt = static_cast<std::string>(user["createdAt"].get_string().value);
				updatedAt = static_cast<std::string>(user["updatedAt"].get_string().value);
				isActive = user["isActive"].get_bool().value;
			}
			catch (...) {

			}
			


			std::vector<std::string> roles;
			auto rolesArray = user["roles"].get_array().value;
			for (const auto& role : rolesArray) {
				roles.push_back(static_cast<std::string>(role.get_string().value));
			}

			users.emplace_back(name, password, roles, isActive);
			users.back().setId(id);
			users.back().setCreatedAt(createdAt);
			users.back().setUpdatedAt(updatedAt);
		}
		return users;
	}

	crow::json::wvalue to_json() const {
		crow::json::wvalue user_json;
		user_json["_id"] = m_id;
		user_json["username"] = m_username;
		user_json["password"] = m_password;
		user_json["active"] = m_active;
		user_json["createdAt"] = m_createdAt;
		user_json["updatedAt"] = m_updatedAt;

		crow::json::wvalue roles_array;

		for (int i = 0; i < m_roles.size(); i++) {
			roles_array[i] = m_roles[i];
		}
		user_json["roles"] = std::move(roles_array);
		
		return user_json;
	}


private:
	std::string m_id;
	std::string m_username;
	std::string m_password;
	bool m_active{ true };
	std::vector<std::string> m_roles;
	std::string m_createdAt{};
	std::string m_updatedAt{};
};