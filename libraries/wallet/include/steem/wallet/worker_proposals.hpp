#pragma once

#include <vector>
#include <string>

#include <steem/plugins/condenser_api/condenser_api_legacy_asset.hpp>

namespace steem { namespace proposal {

        struct CreateProposal {

            static const int create_proposal_args_cnt = 7;

            /* Proposal creator */
            std::string creator;

            /* Funds receiver */
            std::string receiver;

            /* Start date */
            std::string start_date;     //or maybe boost_datetime ??

            /* End date */
            std::string end_date;     //or maybe boost_datetime ??

            /* Proposal daily payment */
            steem::plugins::condenser_api::legacy_asset daily_pay;

            /* Subject of proposal */
            std::string subject;

            /* Url with proposal description */
            std::string url;

            CreateProposal(const std::string& _creator
                        , const std::string& _receiver
                        , const std::string& _start_date
                        , const std::string& _end_date
                        , steem::plugins::condenser_api::legacy_asset _daily_pay
                        , const std::string& _subject
                        , const std::string& _url) : creator{_creator}
                                            , receiver{_receiver}
                                            , start_date{_start_date}
                                            , end_date{_end_date}
                                            , daily_pay{_daily_pay}
                                            , subject{_subject}
                                            , url{_url}
            {}
        };

        struct UpdateProposalVotes {
            
            static const int update_proposal_votes_args_cnt = 3;

            using Proposals = std::vector<int>;

            /* Proposal voter */
            std::string voter;

            /* Voters proposals */
            Proposals proposals;

            /*Proposal(s) approval */
            bool approve;


            UpdateProposalVotes(const std::string& _voter
                            , const std::string& _proposals
                            , bool _approve) : voter(_voter)
                                            , approve(_approve)
            {
                const std::string digits    = "0123456789";
                const std::string delimeter = ",";

                size_t pos  = 0; 
                size_t pose = 0; 
                size_t first_digit = _proposals.find_first_of(digits);
                size_t last_digit  = _proposals.find_last_of(digits);
                if( first_digit != std::string::npos and last_digit != std::string::npos) {
                    std::string pr =   _proposals.substr(first_digit, last_digit); 
                    std::string temp = "";
                    if(!pr.empty()) {
                        while(( pose = pr.find(delimeter, pos) ) != std::string::npos) {
                            temp = pr.substr(pos, pose-pos);
                            if(!temp.empty()){
                                proposals.push_back(std::stoi(temp));
                            }
                            pos = pose + delimeter.length();
                        }
                        proposals.push_back(std::stoi(pr.substr(pos)));
                    }
                }
            }

            UpdateProposalVotes(const std::string& _voter
                            , Proposals _proposals
                            , bool _approve) : voter(_voter)
                                            , approve(_approve)
            {
                proposals = _proposals;
            }
        };
    }
}   //steem::proposals