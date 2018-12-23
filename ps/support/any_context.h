#ifndef PS_SUPPORT_ANY_CONTEXT_H
#define PS_SUPPORT_ANY_CONTEXT_H


namespace ps{
namespace support{

        /*
         * Motivation for this is loose coupling. Sometimes we want to dynamically 
         * add propeirties to an object, but for C++ we still want to keep the 
         * type
         */
        struct AnyContext{
                struct Key{
                        std::type_index decl_type;
                        std::string name;
                        bool operator<(Key const& that)const{
                                if( decl_type != that.decl_type )
                                        return decl_type < that.decl_type;
                                return name < that.name;
                        }
                };
                struct Item{
                        Key key;
                        std::shared_ptr<void> data;
                        bool operator<(Item const& that)const{
                                return key < that.key;
                        }
                        bool operator<(Key const& that)const{
                                return key < that;
                        }
                };
                // there idea here, is that each value decl might be 
                // a mini implementaipon, want to create a new item with this
                struct Maker{
                        void Create(Key const& key,
                                    std::shared_ptr<void> const& ptr)const
                        {
                                self_->items_.push_back(std::make_shared<Item>(Item{key, ptr}));
                                std::sort(self_->items_.begin(), self_->items_.end(),
                                          [](auto lp, auto rp){ return *lp < *rp; });
                        }
                        mutable AnyContext* self_;
                };

                template<class Decl>
                typename std::decay_t<Decl>::ReturnType ValueOrThrow(Decl const& decl){
                        auto key = decl.MakeKey();
                        AnyContext* head = this;
                        for(;!!head;head = head->parent_){
                                auto opt = head->Impl_UpperBound(key);
                                if( opt ){
                                        return decl.Cast(opt->data);
                                }
                        }
                        BOOST_THROW_EXCEPTION(std::domain_error("doesn't exist"));
                }
                
                template<class Decl>
                typename std::decay_t<Decl>::ReturnType Value(Decl const& decl){
                        auto key = decl.MakeKey();
                        auto opt = Impl_UpperBound(key);
                        if(opt){
                                return decl.Cast(opt->data);
                        }
                        decl.CreateOrThrow(this, maker_);
                        // if this is incorrectly set up, with will repeat forever
                        return Value(decl);
                }
                template<class Decl>
                typename std::decay_t<Decl>::ReturnType Add(Decl const& decl){
                        auto key = decl.MakeKey();
                        auto opt = Impl_UpperBound(key);
                        if(opt){
                                BOOST_THROW_EXCEPTION(std::domain_error("already exist"));
                        }
                        decl.CreateOrThrow(this, maker_);
                        return Value(decl);
                }
                template<class Decl>
                typename std::decay_t<Decl>::ReturnType Define(Decl const& decl){
                        auto key = decl.MakeKey();
                        auto opt = Impl_UpperBound(key);
                        if(opt){
                                BOOST_THROW_EXCEPTION(std::domain_error("already exist"));
                        }
                        decl.Create(this, maker_);
                        return Value(decl);
                }

                // this is because we want to use the normal semantics
                // to create a parent
                struct Parent{
                        AnyContext* ptr;
                };
                explicit AnyContext(Parent const& parent){
                        parent_ = parent.ptr;
                }
                AnyContext()=default;
        private:
                boost::optional<Item&> Impl_UpperBound(Key const& key){
                        auto iter = std::lower_bound(items_.begin(), items_.end(), key,
                                                     [](auto l, auto r){ return *l < r; } );
                        if( iter != items_.end())
                                return {**iter};
                        return {};
                }

                std::vector<std::shared_ptr<Item> > items_;
                Maker maker_{this};
                // yes we have parents!
                AnyContext* parent_{nullptr};
        };
        template<class T>
        struct ValueDecl{
                using ReturnType = T&;
                ValueDecl(std::string const& name)
                        : name_{name}, key_{ typeid(T), name}
                {}
                ValueDecl(std::string const& name, T const& init)
                        : name_{name}, init_{init}, key_{ typeid(T), name}
                {}
                ReturnType Cast(std::shared_ptr<void> ptr)const{
                        auto* typed = reinterpret_cast<T*>(ptr.get());
                        return *typed;
                }
                void CreateOrThrow(AnyContext* ctx, AnyContext::Maker const& maker)const{
                        if( init_ ){
                                this->Create(ctx, maker);
                        }
                        BOOST_THROW_EXCEPTION(std::domain_error("no default"));
                }
                void Create(AnyContext* ctx, AnyContext::Maker const& maker)const{
                        maker.Create(key_, std::make_shared<T>(init_.get()));
                }
                AnyContext::Key MakeKey()const{ return key_; }
        private:
                std::string name_;
                boost::optional<T> init_;
                AnyContext::Key key_;
        };

        void AnyContextTest(){
                ValueDecl<int> A("A", 1);
                AnyContext ctx;

                std::cout << "ctx.Value(A) = " << ctx.Value(A) << "\n";
                ++ctx.Value(A);
                std::cout << "ctx.Value(A) = " << ctx.Value(A) << "\n";

                AnyContext child(ctx);

                std::cout << "child.ValueOrThrow(A) = " << child.ValueOrThrow(A) << "\n";
        }
} // end namespace support
} // end namespace ps

#endif // PS_SUPPORT_ANY_CONTEXT_H
