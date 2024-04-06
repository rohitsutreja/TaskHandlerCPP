#include "Util.h"
#include "User.h"
#include <string>
#include <optional>
#include <iostream>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/exception/exception.hpp>

extern mongocxx::database db;


class Note {
public:
	Note(const std::string& userId, const std::string& title, const std::string& text, bool completed = false)
		: m_user(userId), m_title(title), m_text(text), m_completed(completed) {}

	Note(const std::string& id, const std::string& userId, const std::string& title, const std::string& text, bool completed = false)
		:m_id{id}, m_user(userId), m_title(title), m_text(text), m_completed(completed) {}


	const std::string& getId() const {
		return m_id;
	}

	void setId(const std::string& id) {
		m_id = id;
	}

	const std::string& getUser() const {
		return m_user;
	}

	void setUser(const std::string& user) {
		m_user = user;
	}

	const std::string& getTitle() const {
		return m_title;
	}

	void setTitle(const std::string& title) {
		m_title = title;

	}

	const std::string& getText() const {
		return m_text;
	}

	void setText(const std::string& text) {
		m_text = text;

	}

	bool getCompleted() const {
		return m_completed;
	}

	void setCompleted(bool completed) {
		m_completed = completed;
		
	}

	const std::string& getCreatedAt() const {
		return m_createdAt;
	}

	const std::string& getUpdatedAt() const {
		return m_updatedAt;
	}

	void setCreatedAt(const std::string& ca)  {
		this->m_createdAt = ca;
	}

	void setUpdatedAt(const std::string& ua)  {
		this->m_updatedAt =  ua;
	}


	void save() {
		auto timestamp = getLogTimeString();
		auto document = bsoncxx::builder::basic::make_document(
			bsoncxx::builder::basic::kvp("user", bsoncxx::oid(m_user)),
			bsoncxx::builder::basic::kvp("title", m_title),
			bsoncxx::builder::basic::kvp("text", m_text),
			bsoncxx::builder::basic::kvp("completed", m_completed),
			bsoncxx::builder::basic::kvp("createdAt", timestamp),
			bsoncxx::builder::basic::kvp("updatedAt", timestamp)
		);
		auto coll = db["notes"];
		coll.insert_one(document.view());
		std::cout << "Note saved successfully." << std::endl;
		
	}

	void remove() {
		mongocxx::collection coll = db["notes"];

		auto filter = bsoncxx::builder::basic::make_document(
			bsoncxx::builder::basic::kvp("_id", bsoncxx::oid(m_id))
		);

		coll.delete_one(filter.view());
	}

	void update() {
		mongocxx::collection coll = db["notes"];

		auto filter = bsoncxx::builder::basic::make_document(
			bsoncxx::builder::basic::kvp("_id", bsoncxx::oid(m_id))
		);


	
		auto timestamp = getLogTimeString();

		auto update = bsoncxx::builder::basic::make_document(
			bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(
				bsoncxx::builder::basic::kvp("user", bsoncxx::oid(m_user)),
				bsoncxx::builder::basic::kvp("title", m_title),
				bsoncxx::builder::basic::kvp("text", m_text),
				bsoncxx::builder::basic::kvp("completed", m_completed),
				bsoncxx::builder::basic::kvp("updatedAt", timestamp)

			))
		);

		std::cout << bsoncxx::to_json(update.view()) << std::endl;
		coll.update_one(filter.view(), update.view());
	}

	static std::optional<Note> getNote(const std::string& id) {
		mongocxx::collection coll = db["notes"];

		auto ID = bsoncxx::oid(id);

		auto document = bsoncxx::builder::basic::make_document(
			bsoncxx::builder::basic::kvp("_id", ID)
		);

		auto result = coll.find_one(document.view());

		if (result) {
			bsoncxx::document::view view = result->view();

			std::string user = view["user"].get_oid().value.to_string();
			std::string title = static_cast<std::string>(view["title"].get_string().value);
			std::string text = static_cast<std::string>(view["text"].get_string().value);
			bool completed =static_cast<bool>(view["completed"].get_bool().value);
			std::string createdAt = static_cast<std::string>(view["createdAt"].get_string().value);
			std::string updatedAt = static_cast<std::string>(view["updatedAt"].get_string().value);

			Note note = Note(id, user, title, text, completed);
			note.setCreatedAt(createdAt);
			note.setUpdatedAt(updatedAt);

			return note;
		}

		// Return a default-constructed User if not found
		return {};
	}

	static std::vector<Note> getAllNotes() {
		mongocxx::collection coll = db["notes"];

		auto result = coll.find({});

		std::vector<Note> notes{};

		for (auto& note : result) {
			std::string id = note["_id"].get_oid().value.to_string();
			std::string user = note["user"].get_oid().value.to_string();
		
			std::string title = static_cast<std::string>(note["title"].get_string().value);
			std::string text = static_cast<std::string>(note["text"].get_string().value);
			bool completed = static_cast<bool>(note["completed"].get_bool().value);
			std::string createdAt = static_cast<std::string>(note["createdAt"].get_string().value);
			std::string updatedAt = static_cast<std::string>(note["updatedAt"].get_string().value);
	
			notes.emplace_back(id, user, title, text, completed);
			notes.back().setCreatedAt(createdAt);
			notes.back().setUpdatedAt(updatedAt);
		}

		return notes;
	}

	crow::json::wvalue to_json() const {
		crow::json::wvalue note_json;
		note_json["_id"] = m_id;

		//auto user = User::getUser(m_user);
		//if (user) {
		//	note_json["username"] = user->getUserName();
		//}

		note_json["user"]      = m_user;
		note_json["title"]     = m_title;
		note_json["text"]      = m_text;
		note_json["completed"] = m_completed;
		note_json["createdAt"] = m_createdAt;
		note_json["updatedAt"] = m_updatedAt;
		

	
		return note_json;
	}

	
private:
	std::string m_id;

	std::string m_user;
	std::string m_title;
	std::string m_text;
	bool m_completed{ false };

	std::string m_createdAt{};
	std::string m_updatedAt{};
};

