

#include <bsoncxx/document/view.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/json.hpp>
#include "include/Model/User.h"
#include "include/Model/Note.h"
#include "crow_all.h"
#include "bcrypt.h"
#include"openssl/ssl.h"
#include "jwt-cpp/jwt.h"
#include <algorithm>



mongocxx::instance inst{};
mongocxx::client client{ mongocxx::uri{"mongodb+srv://rohit:drbezhkeTNd7rEZL@cluster0.ldjcnlk.mongodb.net/?retryWrites=true&w=majority&appName=Cluster0"} };
mongocxx::database db = client["test"];

struct VerifyJWT;

extern crow::App<crow::CookieParser, crow::CORSHandler,VerifyJWT> app;

struct VerifyJWT : crow::ILocalMiddleware {
    struct context {
        std::string username;
        crow::json::rvalue roles; 
    };


    //void before_handle(crow::request& req, crow::response& res, context& ctx) {
    //    auto token = req.get_header_value("authorization");

    //    auto accesstoken = token.substr(7);

    //    try {
    //                auto decoded = jwt::decode(accesstoken);

    //                 auto verifier = jwt::verify()
    //                    .allow_algorithm(jwt::algorithm::hs256{ "ThisIs256BitSecret" });

    //                try {

    //                    std::cout << decoded.get_payload_claim("UserInfo").as_string();
    //                    verifier.verify(decoded);

    //                    std::string str = decoded.get_payload_claim("UserInfo").as_string();


    //                    std::cout << str;
    //                    crow::json::rvalue data = crow::json::load(str);

    //                    ctx.username = data["username"].s();
    //                    ctx.roles    = data["roles"];               

    //                }
    //                catch (...) {
    //                    std::cout << "Wrong token";
    //                    res.body = "Wrong token";
    //                   // res.end();
    //                }
    //            }
    //            catch (...) {
    //                std::cout << "Invalid Token";
    //            }
    //}


    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        auto& ctx1 = app.get_context<crow::CookieParser>(req);
        std::string token = ctx1.get_cookie("jwt");

        try {
            auto decoded = jwt::decode(token);

             auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{ "ThisIs256BitSecret" });

            try {

                std::cout << decoded.get_payload_claim("UserInfo").as_string();
                verifier.verify(decoded);

                std::string str = decoded.get_payload_claim("UserInfo").as_string();


                std::cout << str;
                crow::json::rvalue data = crow::json::load(str);

                ctx.username = data["username"].s();
                ctx.roles    = data["roles"];               

                std::cout << ctx.roles;
            }
            catch (...) {
                std::cout << "Wrong token";
                res.body = "Wrong token";
                res.end();
            }
        }
        catch (...) {
            std::cout << "Invalid Token";
        }
    }

    void after_handle(crow::request&, crow::response&, context&) {}
};



crow::App<crow::CookieParser, crow::CORSHandler, VerifyJWT> app;


int main() {
  
    auto& cors = app.get_middleware<crow::CORSHandler>();
   

     cors.global().origin("http://localhost:3002");
     cors.global().allow_credentials();
     cors.global().headers("Content-Type","Authorization");
     cors.global().methods("GET"_method, "POST"_method, "DELETE"_method, "PATCH"_method, "OPTIONS"_method); 

    CROW_ROUTE(app, "/auth/")
        .methods("POST"_method)
        ([] (const crow::request& req){
        crow::json::rvalue data = crow::json::load(req.body);

        std::string username;
        std::string password;

        try {
            username = data["username"].s();
            password = data["password"].s();

           
            if (username.empty() || password.empty()) {
                return crow::response{400 ,"All fields required"};
                
            }
        }
        catch (...) {
           
            return crow::response{ 400, "ALl fields required" };
        }


      


        auto foundUser = User::getUserByUserName(username);

        if (!foundUser || !foundUser->getStatus()) {
            return crow::response{ "Unauthorized" };
        }

        if (foundUser->getPassword() != password) {
            return crow::response{ "Unauthorized" };
        }

        //crow::response res;

        //auto refreshtoken = jwt::create().set_type("JWT").set_payload_claim("username", jwt::claim(foundUser->getUserName())).sign(jwt::algorithm::hs256{ "ThisIs256BitSecret" });

        //res.add_header("Set-Cookie", "jwt=" + refreshtoken + "; Max - Age = 360000; Path = / ; Secure; HttpOnly");
        //res.set_header("Content-Type","application/json");



        //std::vector<std::string> roles = foundUser->getRoles(); // replace with actual roles
        //crow::json::wvalue rolesData;

        //int i = 0;
        //for (auto& role : roles) {
        //    rolesData[i] = role;
        //    i++;
        //}

        //crow::json::wvalue UserInfo{ {{"username", foundUser->getUserName()},{"roles", rolesData}} };

        //

        //auto token = jwt::create()
        //    .set_type("JWT")
        //    .set_payload_claim("UserInfo", jwt::claim(UserInfo.dump()))
        //    .sign(jwt::algorithm::hs256{ "ThisIs256BitSecret" });


        //res.body = crow::json::wvalue{ {"accessToken",token} }.dump();

        //return res;


        crow::response res;

        std::vector<std::string> roles = foundUser->getRoles(); // replace with actual roles
        crow::json::wvalue rolesData;

        int i = 0;
        for (auto& role : roles) {
            rolesData[i] = role;
            i++;
        }

        crow::json::wvalue UserInfo{  {{"username", username},{"roles", rolesData} } };
          
        auto token = jwt::create()
            .set_type("JWT")
            .set_payload_claim("UserInfo", jwt::claim(UserInfo.dump()))
            .sign(jwt::algorithm::hs256{ "ThisIs256BitSecret" });

      

        res.add_header("Set-Cookie", "jwt=" + token + "; Max - Age = 360000; Path = / ; Secure; HttpOnly");
        res.set_header("Content-Type","application/json");

        res.body = crow::json::wvalue{ {"accessToken", token }}.dump();

        std::cout << "sent " << res.body;

        return res;


        });


    CROW_ROUTE(app, "/auth/refresh/").methods("GET"_method)([](const crow::request& req) {

        auto& ctx1 = app.get_context<crow::CookieParser>(req);
        std::string refreshToken = ctx1.get_cookie("jwt");

        if (refreshToken.empty()) {
            return crow::response{ 401, "Unautharized" };
        }

        try {
            auto decoded = jwt::decode(refreshToken);

            
            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{ "ThisIs256BitSecret" });

            try {
                verifier.verify(decoded);
                auto user = User::getUserByUserName(decoded.get_payload_claim("UserInfo").to_json().get("username").to_str());

                if (!user) {
                    return crow::response{ 401, "Unauthraized" };
                }

          
                std::vector<std::string> roles = user->getRoles(); // replace with actual roles
                crow::json::wvalue rolesData;

                int i = 0;
                for (auto& role : roles) {
                    rolesData[i] = role;
                    i++;
                }

                crow::json::wvalue UserInfo{ {{"username", user->getUserName()},{"roles", rolesData}} };

                auto token = jwt::create()
                    .set_type("JWT")
                    .set_payload_claim("UserInfo", jwt::claim(UserInfo.dump()))
                    .sign(jwt::algorithm::hs256{ "ThisIs256BitSecret" });



                return crow::response{ crow::json::wvalue{{"accessToken",token}}};

                
            }
            catch (...) {
              
            }
        }
        catch (...) {
            return crow::response{ 403,"Forbidden" };
        }

        });


    //CROW_ROUTE(app, "/auth/logout")
    //    .methods("POST"_method)
    //    ([](const crow::request& req) {
    //        

    //        });



    CROW_ROUTE(app, "/auth/refresh/").methods("OPTIONS"_method).CROW_MIDDLEWARES(app, VerifyJWT)([]() {
        return crow::response{ "OK" }; });

    CROW_ROUTE(app, "/auth/").methods("OPTIONS"_method).CROW_MIDDLEWARES(app, VerifyJWT)([]() {
        return crow::response{ "OK" }; });

    CROW_ROUTE(app, "/users/").methods("OPTIONS"_method).CROW_MIDDLEWARES(app, VerifyJWT)([]() {
        return crow::response{ "OK" }; });

    CROW_ROUTE(app, "/notes/").methods("OPTIONS"_method).CROW_MIDDLEWARES(app, VerifyJWT)([]() {
        return crow::response{ "OK" }; });

    //Get all Users
    CROW_ROUTE(app, "/users/")
        .methods("GET"_method).CROW_MIDDLEWARES(app, VerifyJWT)
        ([] (const crow::request& req){
     
        crow::json::wvalue users;
        std::vector<User> u = User::getAllUsers();

        if (u.size() == 0) return crow::response{ 400, "No User Found" };

        int i = 0;
        for (const auto& user : u) {
            std::cout << i << '\n';
            users[i] = user.to_json();
            i++;
        }


        return crow::response{ users };
    });


    //Delete a User
    CROW_ROUTE(app, "/users/")
        .methods("DELETE"_method).CROW_MIDDLEWARES(app, VerifyJWT)
        ([](const crow::request& req) {
        crow::json::rvalue data = crow::json::load(req.body);
        
        std::string id;
        try {
           id = data["id"].s();

           if (id.empty()) {
               return crow::response{400, "User ID Required" };
           }
        }
        catch (...) {
           return crow::response{ 400, "User ID Required" };
        }
         
        mongocxx::collection coll = db["notes"];

        auto document = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("user", bsoncxx::oid(id)));

        auto result = coll.find_one(document.view());

        if (result) {
           return crow::response{ 400,  "User has assigned notes" };  
        }


        auto user = User::getUser(id);

        if (!user) {
            return crow::response{ "User not found" };
        }

        user->remove();

        return crow::response{ "User removed successfully" };
        });


    //Add new User
    CROW_ROUTE(app, "/users/")
        .methods("POST"_method).CROW_MIDDLEWARES(app, VerifyJWT)
        ([](const crow::request& req) {

        crow::json::rvalue data = crow::json::load(req.body);

        std::string username;
        std::string password;
        crow::json::rvalue rolesData;

        try {
            username = data["username"].s();
            password = data["password"].s();
            rolesData = data["roles"];
            if (username.empty() || password.empty() || rolesData.size() == 0) {
                return crow::response{ "All fields are required" };
            }
        }
        catch (...) {
            return crow::response{ 400, "All fields are required ok" };
        }
        

        //find duplicate
        mongocxx::collection coll = db["users"];
        auto document = bsoncxx::builder::basic::make_document(
            bsoncxx::builder::basic::kvp("username", username)
        );

        auto result = coll.find_one(document.view());

        if (result ) {
            return crow::response{ 409, "Duplicate Username" };
        }

        
        std::vector<std::string> roles;
        for (const auto& v : rolesData) {
            roles.push_back(v.s());
        }

        User u{ username, password, roles };

        if (u.save()) {
            return crow::response{201,  "User created successfully" };
        };

        return crow::response{ 400 , "Invalid User data recieved" };
        });


    //Update a User
    CROW_ROUTE(app, "/users/")
        .methods("PATCH"_method).CROW_MIDDLEWARES(app, VerifyJWT)
        ([](const crow::request& req) {
        crow::json::rvalue data = crow::json::load(req.body);


        std::string id;
        std::string username;
        std::string password;
        bool active = false;
        crow::json::rvalue rolesData;

        try { 
            password = data["password"].s();
        }
        catch (...) {
            password = "";
        }

        try {
            active = data["active"].b();
            id = data["id"].s();
            username = data["username"].s();
            rolesData = data["roles"];

            if (username.empty() || rolesData.size() == 0 || id.empty()) {
                return crow::response{ 400, "All fields are required" };
            }

        }
        catch (...) {
            return crow::response{ 400, "All fields are required" };
        }

        std::vector<std::string> roles;
        for (const auto& v : rolesData) {
            roles.push_back(v.s());
        }

        auto user = User::getUser(id);

        if (!user) {
            return crow::response{ 400, "User not found" };
        }



        //find duplicate
        mongocxx::collection coll = db["users"];
        auto document = bsoncxx::builder::basic::make_document(
            bsoncxx::builder::basic::kvp("username", username)
        );

        auto result = coll.find_one(document.view());

        if (result) {
                bsoncxx::document::view view = result->view();
                if (result && view["_id"].get_oid().value.to_string() != id) {
                    return crow::response{409, "Duplicate Username" };
                }
        }

        user->setUserName(username);
        if (!password.empty()) {
            user->setPassword(password);
        }
    
        user->setStatus(active);
        user->setRoles(roles);

        user->update();


        return crow::response{ "User updated" };
        });

  
    //Get all Notes
    CROW_ROUTE(app, "/notes/")
        .methods("GET"_method).CROW_MIDDLEWARES(app, VerifyJWT)
        ([] {
   
        crow::json::wvalue notes;
        std::vector<Note> allNotes = Note::getAllNotes();

        if (allNotes.size() == 0) {
            return crow::response{ 400, "No notes found" };
        }

        auto users = User::getAllUsers();

        int i = 0;
        for (const auto& note : allNotes) {
            notes[i] = note.to_json();
            notes[i]["username"] = std::find(std::begin(users), std::end(users), User(note.getUser()))->getUserName();
            i++;
        }

        return crow::response{ notes };
            });



    //Delete a Note
    CROW_ROUTE(app, "/notes/")
        .methods("DELETE"_method).CROW_MIDDLEWARES(app, VerifyJWT)
        ([](const crow::request& req) {
        crow::json::rvalue data = crow::json::load(req.body);

        std::string id;
        try {
            id = data["id"].s();

            if (id.empty()) {
                return crow::response{ 400,"Note ID required" };
            }
        }
        catch (...) {
            return crow::response{ 400,"Note ID required" };
        }
     
        auto note = Note::getNote(id);

        if (!note) {
           return crow::response{ 400,"Note not found" };
        }

        note->remove();

        return crow::response{ "Note removed successfully" };
        });


    //Add new Note
    CROW_ROUTE(app, "/notes/")
        .methods("POST"_method).CROW_MIDDLEWARES(app, VerifyJWT)
        ([](const crow::request& req) {
        crow::json::rvalue data = crow::json::load(req.body);

        std::string user;
        std::string title;
        std::string text;

        try {
           user = data["user"].s();
           title = data["title"].s();
           text = data["text"].s();

            if (user.empty() || title.empty() || text.empty()) {
                return crow::response{400, "All fields are required." };
            }
        }
        catch (...) {
            return crow::response{ 400, "All fields are required." };
        }

        mongocxx::collection coll = db["notes"];

        auto document = bsoncxx::builder::basic::make_document(
            bsoncxx::builder::basic::kvp("title", title)
        );

        auto duplicate = coll.find_one(document.view());


        if (duplicate) {
            return crow::response{ 409, "Duplicate note title" };
        }
        Note note{ user,title,text };
        note.save();

        return crow::response{201, "Note created successfully" };
        });


    //Update Note
    CROW_ROUTE(app, "/notes/")
        .methods("PATCH"_method).CROW_MIDDLEWARES(app, VerifyJWT)
        ([](const crow::request& req) {
        std::cout << "\nfirst\n";
        crow::json::rvalue data = crow::json::load(req.body);

        std::cout << "\nsecond\n";
        std::string id;
        std::string user;
        std::string title;
        std::string text;
        bool completed;

        try {
             id = data["id"].s();
             user = data["user"].s();
             title = data["title"].s();
             text = data["text"].s();
             completed = data["completed"].b();

            if (id.empty() || user.empty() || title.empty() || text.empty()) {
                return crow::response{ 400, "All fields are required." };
            }

        }
        catch(...){
            return crow::response{ 400,"All fields are required." };
        }

       

        auto note = Note::getNote(id);

        std::cout << "\nthird\n";

        if (!note) {
            return crow::response{ 400,"Note not found" };
        }

        mongocxx::collection coll = db["notes"];

        auto document = bsoncxx::builder::basic::make_document(
            bsoncxx::builder::basic::kvp("title", title)
        );


        std::cout << "\nfourth\n";
        auto duplicate = coll.find_one(document.view());

        if (duplicate && duplicate.value()["_id"].get_oid().value.to_string() != id) {
            return crow::response{ 409, "Duplicate note title" };
        }

        note->setTitle(title);
        note->setCompleted(completed);
        note->setText(text);
        note->setUser(user);


        std::cout << "\nfifth\n";
        note->update();


        std::cout << "\nsixth\n";
        return crow::response{ "Note updated successfully" };
            });

    app.bindaddr("127.0.0.1").port(5000).run();
}
